#!/usr/bin/env python3
"""Compile all ten source Animals Surprise data variants; exclude native code."""
import json,hashlib,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];sys.path.insert(0,str(root/'Randomizer/upstream'))
from rom.ips import IPS_Patch
names=['animal_enemies','animals','draygonimals','escapimals','gameend','grey_door_animals','low_timer','metalimals','phantoonimals','ridleyimals']
tails={'draygonimals':(0x7f06f,2),'metalimals':(0x7f038,4),'phantoonimals':(0x7f03b,2),'ridleyimals':(0x7f05e,2)}
payload=[];spans=[];modes=[];audit=[]
for name in names:
 path=root/'Randomizer/upstream/patches/vanilla/ips'/(name+'.ips');records=IPS_Patch.load(path).toDict();begin=len(spans);data=[];code=[]
 for a,b in records.items():
  b=bytes(b)
  if a==0x19e6e6 or 0x7f006<=a<0x80000 and name!='grey_door_animals':
   code.append(dict(address=a,size=len(b),hex=b.hex()))
   if name in tails:
    start,n=tails[name]
    if a<=start<a+len(b):data.append((start,b[start-a:start-a+n]))
  else:data.append((a,b))
 for a,b in data:
  spans.append(dict(address=a,offset=len(payload),size=len(b)));payload.extend(b)
 modes.append((begin,len(spans)-begin))
 audit.append(dict(id=len(modes),name=name+'.ips',source=str(path.relative_to(root)),sha256=hashlib.sha256(path.read_bytes()).hexdigest(),data=[dict(address=a,data=list(b)) for a,b in data],nativeCode=code))
catalog=dict(schema=1,upstream='72ec1f30b700442d0c30aad6c43801bd64f1e2a5',variants=audit)
catalog['sha256']=hashlib.sha256(json.dumps(catalog,sort_keys=True,separators=(',',':')).encode()).hexdigest()
from world_rom_assets import build as build_rom_recipe
ranges=[(0,0)]+modes
build_rom_recipe(root,[dict(id=i,name='Animal surprise '+str(i)) for i in range(len(ranges))],ranges,[(s['address'],s['offset'],s['size']) for s in spans],payload,{},domain='animals')
for variant in audit:
 for span in variant['data']:
  raw=bytes(span.pop('data'));span.update(size=len(raw),sha256=hashlib.sha256(raw).hexdigest())
lines=['/* Generated from pinned VARIA Animals Surprise patches; data only. */',f'#define ANIMALS_DATA_SIZE {len(payload)}',
 f'static uint8_t animals_bytes[{len(payload)}];',
 'static const struct {uint32_t address,offset,size;} animals_spans[]={'+','.join('{%d,%d,%d}'%(p['address'],p['offset'],p['size']) for p in spans)+'};',
 'static const struct {uint16_t first,count;} animals_variants[]={{0,0},'+','.join('{%d,%d}'%m for m in modes)+'};',
 'static const char animals_catalog[]="'+catalog['sha256']+'";']
(root/'Native/sm_animals_data.inc').write_text('\n'.join(lines)+'\n')
(root/'Randomizer/native_animals.json').write_text(json.dumps(catalog,indent=2)+'\n')
(root/'Unreal/Source/SMUnreal/SMNativeAnimals.inl').write_text('static const TCHAR* NativeAnimalsCatalog=TEXT(\"'+catalog['sha256']+'\");\nstatic const TCHAR* NativeAnimalNames[]={TEXT(\"\"),'+','.join('TEXT(\"'+x['name']+'\")' for x in audit)+'};\n')
print('ANIMALS_DATA',len(modes),len(payload),catalog['sha256'])
