#!/usr/bin/env python3
"""Audit pinned colored-door PLM data; exclude every injected CPU routine."""
from pathlib import Path
import sys,json,hashlib
root=Path(__file__).resolve().parents[1];upstream=root/'Randomizer/upstream';sys.path.insert(0,str(upstream))
from logic.logic import Logic
Logic.factory('vanilla')
from rom.flavor import RomFlavor
RomFlavor.factory(str(upstream))
from rom.ips import IPS_Patch
from rom.rom import snes_to_pc
from utils.doorsmanager import DoorsManager,colors2plm,colors2plmIndicator
colors=['blue','red','green','yellow','grey','wave','spazer','plasma','ice']
patches={}
for name in ('beam_doors_plms','beam_doors_gfx','red_doors'):
 p=upstream/f'patches/common/ips/{name}.ips'
 patches[name]=dict(sha256=hashlib.sha256(p.read_bytes()).hexdigest(),spans=IPS_Patch.load(str(p)).toDict())
mem={a+i:v for a,b in patches['beam_doors_plms']['spans'].items() for i,v in enumerate(b)}
def word(pointer):return mem[0x18000+pointer]|mem[0x18001+pointer]<<8
records=[];checks={}
for color in colors[5:]:
 for direction,plm in enumerate(colors2plm[color]):
  main,close=word(plm+2),word(plm+4)
  assert word(plm)==0xc7b1 and word(main)==0x8a72 and word(main+8)==0x86c1
  check=word(main+10);checks[color]=check
  records.append(dict(color=color,direction=direction,plm=plm,main=main,close=close,check=check,draw=word(main+14),indicator=False))
for color in colors[5:]:
 for direction,plm in enumerate(colors2plmIndicator[color]):
  main,close=word(plm+2),word(plm+4)
  assert word(plm)==0xc7b1 and word(main+10)==0xbd0f
  records.append(dict(color=color,direction=direction,plm=plm,main=main,close=close,indicator=True))
assert checks==dict(wave=0xf2db,spazer=0xf2f9,plasma=0xf31b,ice=0xf2ea)
code_start=min(checks.values());code_end=min(r['close'] for r in records if r['indicator'])
assert (code_start,code_end)==(0xf2db,0xf35b)
# The only native-code ranges in these patches. All other bytes are PLM
# lists/draw blocks, original draw edits, or compressed graphics/table data.
excluded={'beam_doors_plms':[(0x18000+code_start,code_end-code_start)],'red_doors':[(0x20560,43)],'beam_doors_gfx':[]}
spans=[];data=[];omitted=[]
for name,patch in patches.items():
 for a,values in sorted(patch['spans'].items()):
  run=[];start=a
  for i,v in enumerate(values):
   address=a+i;code=any(x<=address<x+n for x,n in excluded[name])
   if code:
    if run:spans.append(dict(address=start,offset=len(data),size=len(run),patch=name));data.extend(run);run=[]
    omitted.append((address,v));start=address+1
   else:
    if not run:start=address
    run.append(v)
  if run:spans.append(dict(address=start,offset=len(data),size=len(run),patch=name));data.extend(run)
assert len(omitted)==171
locations=[]
for name,d in sorted(DoorsManager.doors.items()):
 locations.append(dict(id=len(locations),name=name,address=snes_to_pc(d.address),facing=int(d.facing),openedBit=d.id,vanillaColor=d.vanillaColor,canRandom=d.canRandom,canGrey=d.canGrey,forbiddenColors=d.forbiddenColors or []))
catalog=dict(schema=1,upstream='72ec1f30b700442d0c30aad6c43801bd64f1e2a5',colors=colors,locations=locations,records=records,
 patches={k:dict(sha256=v['sha256'],excludedCode=excluded[k]) for k,v in patches.items()},spans=spans,dataSha256=hashlib.sha256(bytes(data)).hexdigest())
catalog['sha256']=hashlib.sha256(json.dumps(catalog,sort_keys=True,separators=(',',':')).encode()).hexdigest()
# Once released, color IDs, location IDs and source patch semantics are fixed.
for path in (root/'Randomizer/door_color_catalog_history').glob('*.json'):
 assert json.loads(path.read_text())==json.loads(json.dumps(catalog)), 'Published door-color catalog changed: '+path.name
lines=['/* Generated from pinned VARIA colored-door patches; CPU code excluded. */',f'#define DOOR_COLOR_COUNT {len(locations)}',f'#define DOOR_DATA_SIZE {len(data)}',
 'static const char door_catalog[]="'+catalog['sha256']+'";', 'static const uint8_t door_data[]={'+','.join(map(str,data))+'};',
 'static const struct {uint32_t address,offset,size;} door_spans[]={']
lines+=['{%d,%d,%d},'%(s['address'],s['offset'],s['size']) for s in spans];lines+=['};','static const struct {uint32_t address;uint8_t facing,can_random,opened_bit;} door_locations[]={']
lines+=['{%d,%d,%d,%d},'%(d['address'],d['facing'],d['canRandom'],d['openedBit']) for d in locations];lines+=['};','static const uint16_t color_plms[9][4]={{0},']
lines+=['{'+','.join(map(str,colors2plm[c]))+'},' for c in colors[1:]];lines+=['};','static const struct {uint16_t plm;uint8_t direction,indicator;} beam_plms[]={']
lines+=['{%d,%d,%d},'%(d['plm'],d['direction'],d['indicator']) for d in records];lines+=['};']
ui=['// Generated immutable native door identities for plan validation.',
    'static const struct {const TCHAR *Name;uint32 Address;uint8 Facing,OpenedBit;bool Randomizable;} NativeDoorSpecs[]={']
for d in locations:
 snes=((d['address']//0x8000|0x80)<<16)|(d['address']%0x8000|0x8000)
 ui.append('{TEXT("%s"),%d,%d,%d,%s},'%(d['name'],snes,d['facing'],d['openedBit'],'true' if d['canRandom'] else 'false'))
ui.append('};')
(root/'Unreal/Source/SMUnreal/SMNativeDoorCatalog.inl').write_text('\n'.join(ui)+'\n')
(root/'Native/sm_doors.inc').write_text('\n'.join(lines)+'\n')
(root/'Randomizer/native_door_colors.json').write_text(json.dumps(catalog,indent=2)+'\n')
out=root/'Docs/Randomizer/FullOptions/DoorColors';out.mkdir(exist_ok=True)
(out/'audit.json').write_text(json.dumps(dict(catalog=catalog,excludedCodeBytes=[dict(address=a,value=v) for a,v in omitted]),indent=2)+'\n')
print('DOOR_COLOR_CATALOG',len(locations),'locations',len(data),'data bytes',len(omitted),'CPU bytes excluded')
