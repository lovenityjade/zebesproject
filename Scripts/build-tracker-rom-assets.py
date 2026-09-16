#!/usr/bin/env python3
"""Describe original ROM pickup/portrait compositions, without storing their pixels.

No PopTracker image files are inputs. Sprite shapes and palettes are read from
bank 89, the common room elements, Tourian's statues and Mother Brain's original
spritemap. Generated data contains whole-tile references and crop coordinates.
"""
from pathlib import Path
import hashlib,json,sys
from world_rom_assets import pc,decode
ROOT=Path(__file__).resolve().parents[1]
rom=(ROOT/'roms/Super Metroid (Japan, USA) (En,Ja).sfc').read_bytes()
assert hashlib.sha1(rom).hexdigest()=='da957f0d63d14cb441d215462904c4fa8519c613'
def word(a):return int.from_bytes(rom[pc(a):pc(a)+2],'little')
def long(a):return int.from_bytes(rom[pc(a):pc(a)+3],'little')
def packed(a):return decode(rom,pc(a))[0]

def tileset(index):
 a=0x8f0000|word(0x8fe7a7+index*2)
 return long(a),long(a+3),long(a+6)

sources=[];ops=[];icons=[]
def source(address,compressed):
 entry=(address,int(compressed))
 if entry not in sources:sources.append(entry)
 return sources.index(entry)
def tile(gfx,off,pal,poff,x,y,flip=0):
 ops.append((gfx,off,pal,poff,x,y,flip))
def icon(index,crop,draw):
 first=len(ops);draw();icons.append(dict(id=index,first=first,count=len(ops)-first,crop=crop))
base_palette=source(tileset(6)[2],True) # Original neutral equipment palette (also used in Tourian).
item_blocks={8:0,7:1,12:2,2:3,4:4,17:5,9:6,3:7,16:8,20:9,13:10,1:11,6:12,11:13,19:14,15:15,25:16}
for index,block in item_blocks.items():
 address=0x898000+block*256
 at=rom.find(bytes([0x64,0x87,0,0x80+block]),pc(0x848000),pc(0x858000))
 assert at>=0
 palette_ids=rom[at+4:at+12]
 def draw():
  for q in range(4):tile(source(address,False),q*32,base_palette,(palette_ids[q]&7)*32,q%2*8,q//2*8)
 icon(index,(0,0,16),draw)
# Common pickup PLM frame-zero draw definitions: tank, missile, super, power bomb.
cre=source(0xb98000,True);definitions=packed(0xb9a09d)
for index,draw_address in [(24,0x84a2df),(21,0x84a2eb),(22,0x84a2f7),(23,0x84a303)]:
 block=word(draw_address+2)&1023
 entries=[int.from_bytes(definitions[block*8+q*2:block*8+q*2+2],'little') for q in range(4)]
 def draw():
  for q,entry in enumerate(entries):
   assert 640<=entry&1023<1024
   tile(cre,((entry&1023)-640)*32,base_palette,((entry>>10)&7)*32,q%2*8,q//2*8,(entry>>14)&3)
 icon(index,(0,0,16),draw)
# Original statue room background; crop heads from its normal first room state.
state=0x8fa677;level=packed(long(state));width=16
size=int.from_bytes(level[:2],'little');blocks=[int.from_bytes(level[2+size+size//2+i:4+size+size//2+i],'little') for i in range(0,size,2)]
index=rom[pc(state)+3];table_addr,gfx_addr,pal_addr=tileset(index)
table=bytearray(8192);common=packed(0xb9a09d);specific=packed(table_addr)
table[:len(common)]=common;table[2048:2048+len(specific)]=specific
gfx_size=len(packed(gfx_addr));gfx=source(gfx_addr,True);pal=source(pal_addr,True)
def background(crop):
 cx,cy,size=crop
 for by in range(cy//16,(cy+size-1)//16+1):
  for bx in range(cx//16,(cx+size-1)//16+1):
   block=blocks[by*width+bx];bid=block&1023;hf=bool(block&0x400);vf=bool(block&0x800)
   for q in range(4):
    srcq=q^(1 if hf else 0)^(2 if vf else 0)
    entry=int.from_bytes(table[bid*8+srcq*2:bid*8+srcq*2+2],'little');t=entry&1023
    common=t*32>=gfx_size
    tile(cre if common else gfx,(t-640 if common else t)*32,pal,((entry>>10)&7)*32,bx*16+q%2*8,by*16+q//2*8,((entry>>14)&3)^(1 if hf else 0)^(2 if vf else 0))
for index,crop in [(10,(94,100,32)),(14,(146,104,32))]:icon(index,crop,lambda c=crop:background(c))
# OBJ sprite atlas placement follows LoadEnemiesToVram. Every sprite tile is
# referenced as a whole tile, including flips and the original OAM layering.
def sprite(spritemap,placements,palette_address,cx=0,cy=0):
 count=word(spritemap)
 for i in reversed(range(count)):
  at=spritemap+2+i*5;w=word(at);x=w&511;x=x-512 if x&256 else x
  y=rom[pc(at)+2];y=y-256 if y&128 else y;entry=word(at+3);size=16 if w&0x8000 else 8
  hf=bool(entry&0x4000);vf=bool(entry&0x8000)
  for by in range(size//8):
   for bx in range(size//8):
    sx=size//8-1-bx if hf else bx;sy=size//8-1-by if vf else by
    tid=(entry&511)+sx+sy*16
    address=next(addr+(tid-first)*32 for first,total,addr in placements if first<=tid<first+total)
    tile(source(address,False),0,source(palette_address,False),0,cx+x+bx*8,cy+y+by*8,(entry>>14)&3)
statue=[(0x100,0xb0,0x87b364),(0xd0,0x30,0x87ad64)]
icon(0,(102,71,32),lambda:sprite(0x8d9192,statue,0xaad745,142,85))
icon(5,(116,121,32),lambda:sprite(0x8d9207,statue,0xaad765,132,136))
icon(18,(-24,-28,56),lambda:sprite(0xa9a586,[(0x100,0x80,0xb78000),(0xd0,0x30,0xb0e800)],0xa99472))
assert sorted(i['id'] for i in icons)==list(range(26))
lines=['/* Generated ROM tile/crop references; never an extracted pixel atlas. */',
       'static const struct {uint32_t address;uint8_t compressed;} tracker_sources[]={']
lines+=['{0x%x,%d},'%s for s in sources];lines+=['};','static const SmTrackerTile tracker_tiles[]={']
lines+=['{'+','.join(map(str,t))+'},' for t in ops]
lines+=['};','static const struct {uint16_t first,count;int16_t x,y,size;} tracker_compositions[26]={']
lines+=['{'+','.join(map(str,[i['first'],i['count'],*i['crop']]))+'},' for i in sorted(icons,key=lambda i:i['id'])]
lines+=['};']
text='\n'.join(lines)+'\n';(ROOT/'Native/sm_tracker_rom_assets.inc').write_text(text)
# Keep map coordinates/IDs intact. The generator's only migration of the old
# include is replacing its initialized pixels with ROM-backed runtime storage.
p=ROOT/'Native/sm_tracker_assets.inc';old=p.read_text();begin=old.index('static const uint8_t tracker_positions')
p.write_text('/* ROM-decoded icons; canonical tracker check coordinates. */\nstatic uint8_t tracker_icons[26][16*16*4];\n'+old[begin:])
proof=dict(schema=1,sourceRomSha1=hashlib.sha1(rom).hexdigest(),icons=26,tileCompositions=len(ops),sourceReferences=len(sources),recipeSha256=hashlib.sha256(text.encode()).hexdigest(),sourcePolicy='Original pickup frames and original Tourian statue/Mother Brain portraits, decoded only from a verified player ROM',packImagesRequired=False,visualReviewPending=True)
(ROOT/'Docs/Releases/ALPHA-0.24/tracker-rom-reconstruction.json').write_text(json.dumps(proof,indent=2)+'\n')
print(json.dumps(proof))
