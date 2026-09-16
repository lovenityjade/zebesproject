#!/usr/bin/env python3
"""Extract authored escape enemy tables, excluding executable IPS hunks."""
import hashlib,json,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];up=root/'Randomizer/upstream';sys.path.insert(0,str(up))
from rom.ips import IPS_Patch
from rom.rom import snes_to_pc
written={};sources={}
for rel in ['patches/common/ips/rando_escape_common.ips','patches/vanilla/ips/rando_escape.ips']:
 p=up/rel;sources[rel]=hashlib.sha256(p.read_bytes()).hexdigest()
 for a,b in IPS_Patch.load(p).toDict().items():written.update((a+i,v) for i,v in enumerate(b))
def word(a):return written[a]|written[a+1]<<8
address=snes_to_pc(0xa1f002);table=[]
while word(address)!=65535:
 table.append(dict(room=word(address),population=word(address+2),graphics=word(address+4)));address+=6
assert len(table)==15
populations=[]
for p in table:
 if p['population']<0xf000:continue
 a=snes_to_pc(0xa10000|p['population']);data=[written[a+i] for i in range(19)]
 assert data[16:]==[255,255,0] and data[:2]==[63,215]
 populations.append(dict(address=a,data=data))
assert len(populations)==6
lines=['/* Pinned source escape elevator populations: data only. */','static const struct {uint16_t room,population,graphics;} escape_enemies[]={']
lines+=['{%d,%d,%d},'%(p['room'],p['population'],p['graphics']) for p in table]
lines+=['};','static const struct {uint32_t address;uint8_t data[19];} escape_populations[]={']
lines+=['{%d,{%s}},'%(p['address'],','.join(map(str,p['data']))) for p in populations]
lines+=['};']
# Reviewed data only: grey-door PLMs, Flyway pairs/table and BT table word.
# Deliberately exclude the adjacent 65816 room-setup and exit instructions.
room_spans=[]
for snes,size in [(0x8fc828,1),(0x8fc836,1),(0x8f81b1,1),(0x8f804f,1),(0x8fc2fa,1),
 (0x8f8b06,6),(0x8fc547,6),(0x8f84b8,6),(0x8ff000,16),(0x8ff046,2)]+[(0x83ada0+24*i,12) for i in range(4)]:
 a=snes_to_pc(snes);room_spans.append(dict(address=a,data=[written[a+i] for i in range(size)]))
assert room_spans[9]['data']==[0,0xae]
# Wrecked Ship live-state level: the IPS starts 19 bytes into the stream,
# preserving its header. It changes four solid blocks to shot blocks and BTS.
from rom.rom import FakeROM
from rom.compression import Compressor
rom=(root/'roms/Super Metroid (Japan, USA) (En,Ja).sfc').read_bytes()
assert hashlib.sha256(rom).hexdigest()=='12b77c4bc9c1832cee8881244659065ee1d84c70c3d29e6eaf92e6798cc2ca72'
ws_path=up/'patches/vanilla/ips/rando_escape_ws_fix.ips';sources[str(ws_path.relative_to(up))]=hashlib.sha256(ws_path.read_bytes()).hexdigest()
ws=IPS_Patch.load(ws_path).toDict();patched=bytearray(rom)
for a,b in ws.items():patched[a:a+len(b)]=bytes(b);room_spans.append(dict(address=a,data=b))
assert rom[0x7cb22:0x7cb25]==bytes.fromhex('c0 bd c4')
old_size,old=Compressor().decompress(FakeROM(dict(enumerate(rom))),0x223dc0)
new_size,new=Compressor().decompress(FakeROM(dict(enumerate(patched))),0x223dc0)
changes=[dict(offset=i,before=a,after=b) for i,(a,b) in enumerate(zip(old,new)) if a!=b]
assert (old_size,new_size,len(old),len(new))==(5063,2761,36866,36866)
assert [(c['offset'],c['before'],c['after']) for c in changes]==[(20697,129,193),(20699,129,193),(20889,129,193),(20891,129,193),(34925,0,11),(34926,0,11),(35021,0,11),(35022,0,11)]
# Source blank map words occupy identical locations in the original tilemaps.
map_path=up/'patches/vanilla/ips/map_data_escape_rando.ips';sources[str(map_path.relative_to(up))]=hashlib.sha256(map_path.read_bytes()).hexdigest()
map_bytes={a+i:v for a,b in IPS_Patch.load(map_path).toDict().items() for i,v in enumerate(b)}
map_tiles=[]
for area,region,offset in [(1,2,0x150),(1,2,0x14e),(1,2,0x14c),(1,2,0x14a),(2,6,0x152),(3,4,0x51a),(4,9,0x4a2)]:
 ptr=int.from_bytes(rom[0x1164a+area*3:0x1164d+area*3],'little');a=snes_to_pc(ptr)+offset
 assert [map_bytes.pop(a),map_bytes.pop(a+1)]==[31,0]
 room_spans.append(dict(address=a,data=[31,0]));index=offset//2
 map_tiles.append(dict(area=area,region=region,byte=index//8,mask=0x80>>(index%8)))
assert not map_bytes
data=[];lines+=['static const struct {uint32_t address;uint16_t size,offset;} escape_room_spans[]={']
for span in room_spans:
 lines+=['{%d,%d,%d},'%(span['address'],len(span['data']),len(data))];data+=span['data']
from world_rom_assets import build as build_rom_recipe
recipe_spans=[];offset=0
for span in room_spans:
 recipe_spans.append((span['address'],offset,len(span['data'])));offset+=len(span['data'])
build_rom_recipe(root,[dict(id=0,name='Escape room changes')],[(0,len(recipe_spans))],recipe_spans,data,{},domain='escape')
lines+=['};',f'#define ESCAPE_ROOM_DATA_SIZE {len(data)}',f'static uint8_t escape_room_data[{len(data)}];']
lines+=['static const struct {uint8_t area,region,byte,mask;} escape_map_tiles[]={'+','.join('{%d,%d,%d,%d}'%(t['area'],t['region'],t['byte'],t['mask']) for t in map_tiles)+'};']
(root/'Native/sm_escape_data.inc').write_text('\n'.join(lines)+'\n')
for span in room_spans:
 payload=bytes(span.pop('data'));span.update(size=len(payload),sha256=hashlib.sha256(payload).hexdigest())
out=root/'Docs/Randomizer/FullOptions/Escape';out.mkdir(parents=True,exist_ok=True)
(out/'data-audit.json').write_text(json.dumps(dict(sources=sources,enemyTable=table,populations=populations,roomSpans=room_spans,mapTiles=map_tiles,wsLevel=dict(state=0xcb22,address=0x223dc0,oldCompressedSize=old_size,newCompressedSize=new_size,decompressedSize=len(new),changes=changes)),indent=2)+'\n')
print('ESCAPE_DATA',len(table),len(populations)*19)
