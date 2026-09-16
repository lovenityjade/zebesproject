#!/usr/bin/env python3
"""Compile source pose data and pickup sound choices; never install SNES code."""
import hashlib,json,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'Randomizer/upstream'))
from rom.ips import IPS_Patch
rom=(root/'roms/Super Metroid (Japan, USA) (En,Ja).sfc').read_bytes()
assert hashlib.sha256(rom).hexdigest()=='12b77c4bc9c1832cee8881244659065ee1d84c70c3d29e6eaf92e6798cc2ca72'
shadow=bytearray(rom)
ips=root/'Randomizer/upstream/patches/common/ips'
respin=IPS_Patch.load(ips/'spinjumprestart.ips').toDict()
for a,b in respin.items():shadow[a:a+len(b)]=bytes(b)
def word(a):return int.from_bytes(shadow[a:a+2],'little')
entries=[];offsets=[];poses=[]
for pose in range(253):
 ptr=word(0x89ee2+pose*2);offsets.append(len(entries));rows=[]
 for i in range(128):
  a=0x88000+(ptr&0x7fff)+i*6;n=word(a)
  if n==65535:entries.append([65535,0,0]);break
  row=[n,word(a+2),word(a+4)];assert row[2]<253
  entries.append(row);rows.append(row)
 else:raise ValueError('Unterminated pose table')
 poses.append(dict(pose=pose,source=ptr,entries=rows))
sounds=IPS_Patch.load(ips/'itemsounds.ips').toDict()
rows=[]
# Three native counterparts for the source's SOUNDFX/SPECIALFX/MISCFX routines.
targets={}
code=sounds[0x26fd3]
for i in range(len(code)-7):
 if code[i]==0x20:
  body=bytes(code[i:i+16])
  for group,call in [(1,b'\x22\x49\x90\x80'),(2,b'\x22\xcb\x90\x80'),(3,b'\x22\x4d\x91\x80')]:
   # Only routine entries start with JSR SETFX and end in RTS after queueing.
   if call in body and body.index(call)+4<len(body) and body[body.index(call)+4]==0x60:
    targets[0xefd3+i]=group;break
for a,b in sounds.items():
 if 0x26000<=a<0x26f00:
  assert len(b)==3 and rom[a:a+2]==b'\xdd\x8b'
  target=b[0]|b[1]<<8;assert target in targets,(hex(target),targets)
  rows.append(dict(address=(a&0xffff)+2,group=targets[target],sound=b[2]))
assert len(rows)==63 and len({r['address'] for r in rows})==63
lines=['/* Generated from pinned VARIA pose data and item sound choices. */',
 'static const uint16_t respin_offsets[253]={'+','.join(map(str,offsets))+'};',
 'static const uint16_t respin_entries[][3]={'+','.join('{'+','.join(map(str,e))+'}' for e in entries)+'};',
 'static const struct {uint16_t address;uint8_t group,sound;} pickup_sounds[]={'+','.join('{%d,%d,%d}'%(r['address'],r['group'],r['sound']) for r in rows)+'};']
(root/'Native/sm_movement_pickup_data.inc').write_text('\n'.join(lines)+'\n')
out=root/'Docs/Randomizer/FullOptions/MovementPickups';out.mkdir(parents=True,exist_ok=True)
(out/'source-data.json').write_text(json.dumps(dict(poses=poses,sounds=rows,sourceSha256={n:hashlib.sha256((ips/(n+'.ips')).read_bytes()).hexdigest() for n in ['spinjumprestart','itemsounds']}),indent=2)+'\n')
print('Generated',len(entries),'pose entries and',len(rows),'pickup sounds')
