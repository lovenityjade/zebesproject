"""Read original room definitions and tiles. Never write to the ROM."""
import ast
import functools
import hashlib
import io
import struct
import sys
from pathlib import Path
from PIL import Image

ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'Randomizer/upstream'))
from rom.compression import Compressor

ROM_SHA256='12b77c4bc9c1832cee8881244659065ee1d84c70c3d29e6eaf92e6798cc2ca72'
AREAS=['Crateria','Brinstar','Norfair','Wrecked Ship','Maridia','Tourian','Ceres']
def pc(address):return ((address>>16)&127)*32768+(address&32767)
class MemoryROM:
    def __init__(self,data):self.data=data;self.position=0
    def seek(self,address):self.position=address
    def readByte(self):
        value=self.data[self.position];self.position+=1;return value

class Assets:
    def __init__(self,rom_path=None):
        self.rom=(Path(rom_path) if rom_path else ROOT/'roms/Super Metroid (Japan, USA) (En,Ja).sfc').read_bytes()
        if hashlib.sha256(self.rom).hexdigest()!=ROM_SHA256:raise ValueError('ROM Japan/USA originale requise.')
        tree=ast.parse((ROOT/'Randomizer/upstream/tools/rooms.py').read_text())
        rows=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='rooms' for t in n.targets))
        self.rooms={}
        for row in rows:
            address=row['Address'];room=address&65535;header=self.rom[address:address+11]
            if len(header)!=11 or not 0<=header[1]<7:continue
            states=[];cursor=address+11
            for _ in range(32):
                condition=self.word(cursor)
                if condition==0xe5e6:
                    states.append(dict(id=(cursor+2)&65535,label='État normal'));break
                if condition not in (0xe612,0xe629,0xe5ff,0xe640,0xe652,0xe669,0xe676):break
                size=5 if condition in (0xe612,0xe629) else 4
                state=self.word(cursor+size-2)
                label={0xe612:'Événement',0xe629:'Boss',0xe5ff:'Tourian',0xe640:'Morph Ball',0xe652:'Morph + missiles',0xe669:'Power Bomb',0xe676:'Speed Booster'}[condition]
                if size==5:label+=f' {self.rom[cursor+2]:02X}'
                states.append(dict(id=state,label=label));cursor+=size
            if not states:continue
            self.rooms[room]=dict(id=room,name=row['Name'],zone=AREAS[header[1]],area=header[1],
                width=header[4]*16,height=header[5]*16,states=states,supported=header[1]!=6)

    def word(self,address):return int.from_bytes(self.rom[address:address+2],'little')
    def long(self,address):return int.from_bytes(self.rom[address:address+3],'little')
    @functools.lru_cache(maxsize=128)
    def decompress(self,address):
        result=Compressor().decompress(MemoryROM(self.rom),pc(address))
        if not result:raise ValueError('Données compressées incomplètes.')
        return bytes(result[1])

    @functools.lru_cache(maxsize=64)
    def tileset(self,index):
        if not 0<=index<29:raise ValueError('Tileset non pris en charge.')
        address=pc(0x8f0000|self.word(pc(0x8fe7a7)+index*2))
        definitions=bytearray(8192);common=self.decompress(0xb9a09d)
        definitions[:len(common)]=common
        specific=self.decompress(self.long(address));definitions[2048:2048+len(specific)]=specific
        graphics=bytearray(32768)
        common=self.decompress(0xb98000);graphics[0x5000:0x5000+len(common)]=common
        specific=self.decompress(self.long(address+3));graphics[:len(specific)]=specific
        palette=self.decompress(self.long(address+6))
        words=struct.unpack('<4096H',bytes(definitions[:8192]))
        atlas=Image.new('RGBA',(512,512));pixels=atlas.load()
        for tile in range(1024):
            for part,entry in enumerate(words[tile*4:tile*4+4]):
                base=(entry&1023)*32;pal=((entry>>10)&7)*16
                for y in range(8):
                    row=7-y if entry&0x8000 else y
                    for x in range(8):
                        bit=x if entry&0x4000 else 7-x
                        ci=sum(((graphics[base+row*2+(plane&1)+(16 if plane>=2 else 0)]>>bit)&1)<<plane for plane in range(4))
                        color=int.from_bytes(palette[(pal+ci)*2:(pal+ci)*2+2],'little')
                        rgb=tuple(((color>>shift)&31)*8+(((color>>shift)&31)>>2) for shift in (0,5,10))
                        pixels[(tile%32)*16+(part%2)*8+x,(tile//32)*16+(part//2)*8+y]=(*rgb,255 if ci else 0)
        data=io.BytesIO();atlas.save(data,format='PNG')
        return dict(png=data.getvalue(),definitions=bytes(definitions[:8192]),graphics=bytes(graphics),palette=palette)

    @functools.lru_cache(maxsize=128)
    def room(self,room,state=None):
        if room not in self.rooms:raise ValueError('Salle inconnue.')
        info=self.rooms[room]
        if not info['supported']:raise ValueError('Les décors Mode 7 de Ceres nécessitent un éditeur distinct.')
        if state is None:state=info['states'][-1]['id']
        if state not in [s['id'] for s in info['states']]:raise ValueError('État inconnu pour cette salle.')
        address=pc(0x8f0000|state);tileset=self.rom[address+3]
        level=self.decompress(self.long(address));size=int.from_bytes(level[:2],'little');count=size//2
        visible=info['width']*info['height']
        # Double Chamber and Bowling Alley store unused extra rows. Native
        # loading uses the stored size for BTS/BG2 offsets, but room dimensions
        # determine visible rows. Preserve that distinction.
        if count<visible:raise ValueError('Dimensions de salle incohérentes.')
        foreground=list(struct.unpack(f'<{count}H',level[2:2+size]))[:visible]
        back=level[2+size+count:2+size*2+count]
        background=list(struct.unpack(f'<{count}H',back))[:visible] if len(back)==size else []
        self.tileset(tileset)
        return dict(**info,state=state,tileset=tileset,foreground=foreground,background=background,
                    collision=[v>>12 for v in foreground],bts=list(level[2+size:2+size+visible]),
                    used=sorted({v&1023 for v in foreground+background}),romSha256=ROM_SHA256)

    def validate_patch(self,data):
        if set(data)-{'room','state','cells'}:raise ValueError('Seules les retouches visuelles sont acceptées.')
        room=self.room(data['room'],data['state']);cells=data['cells']
        if not isinstance(cells,list) or len(cells)>100000:raise ValueError('Trop de cases.')
        result={}
        for cell in cells:
            if not isinstance(cell,dict) or set(cell)!={'x','y','layer','tile'}:raise ValueError('Case invalide.')
            if any(type(v) is not int for v in cell.values()):raise ValueError('Coordonnées et tiles entières requises.')
            x,y,layer,tile=(cell[k] for k in ('x','y','layer','tile'))
            if not (-128<=x<=511 and -128<=y<=511 and layer in (0,1) and 0<=tile<4096):raise ValueError('Case hors limites.')
            expected=-1
            blocks=room['foreground' if layer==0 else 'background']
            if blocks and 0<=x<room['width'] and 0<=y<room['height']:expected=blocks[y*room['width']+x]
            result[(layer,x,y)]=dict(cell,expected=expected)
        return dict(version=1,romSha256=ROM_SHA256,room=room['id'],state=room['state'],tileset=room['tileset'],
                    cells=[result[k] for k in sorted(result)])
