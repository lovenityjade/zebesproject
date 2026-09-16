#!/usr/bin/env python3
"""Compile reviewed pinned ROM-data patches; exclude executable SNES changes.
The runtime receives only IDs, never arbitrary ROM addresses or patch bytes.
"""
from pathlib import Path
import json,hashlib
root=Path(__file__).resolve().parents[1]
from world_rom_assets import load_dependencies
audit=load_dependencies(root)
# These contain native routine adaptations, never installed instruction bytes.
code={'door_transition.ips','wh_open_tube.ips','LN_Chozo_SpaceJump_Check_Disable','bomb_torizo.ips'}
indicators=json.loads((root/'Docs/Randomizer/FullOptions/DoorIndicators/audit.json').read_text())
# Code-only selections have immutable IDs too, but never install instruction
# bytes. The generated macros bind the audited C counterparts to those IDs.
native_code={'LN_Chozo_SpaceJump_Check_Disable':'LN_CHOZO', 'bomb_torizo.ips':'BOMB_TORIZO'}
patches=[];data=[];spans=[];metadata=[]
history=[json.loads(p.read_text()) for p in sorted((root/'Randomizer/world_catalog_history').glob('*.json'))]
names=list(dict.fromkeys(p['name'] for old in history for p in old['patches']))
names += [name for name in audit['patches'] if name not in names]
for name in names:
 patch=audit['patches'][name]
 if name in code and name not in native_code:continue
 begin=len(spans)
 for span in ([] if name in native_code else patch['spans']):
  # Reviewed domains: load records/map icons, room/door definitions, enemy
  # populations and compressed level data. A domain alone is not approval;
  # definitions are pinned to this explicit source audit and its digest.
  address=span['address'];values=span['data']
  assert hashlib.sha256(bytes(values)).hexdigest()==span['sha256']
  assert 0<=address<0x300000 and address+len(values)<=0x300000
  offset=len(data);data.extend(values);spans.append((address,offset,len(values)))
 patches.append((begin,len(spans)-begin))
 metadata.append(dict(id=len(patches)-1,name=name,source=patch['source'],spans=[] if name in native_code else [{k:v for k,v in x.items() if k!='data'} for x in patch['spans']],additionalPlm=patch['additionalPlm']))
 if name=='door_indicators_plms.ips':metadata[-1]['indicatorLocations']=indicators['locations']
 if name in native_code:
  metadata[-1]['nativeBehavior']=native_code[name]
  # Retain the exact source-code audit even though its bytes are not installed.
  metadata[-1]['sourceSpans']=[{k:v for k,v in x.items() if k!='data'} for x in patch['spans']]
assert len(patches)<=64
# Indicator entry restoration is independent of the ordered room-data spans.
# Reject a new overlap until its ordering/restoration is explicitly integrated.
for loc in indicators['locations']:
 for address,entry in (loc['romReplacements'] or {}).items():
  assert all(int(address)+len(entry)<=a or int(address)>=a+n for a,_,n in spans)
# Boss routing owns a separate restoration domain. World patches currently
# do not overlap it; reject future overlaps until ordering is made explicit.
connections=json.loads((root/'Randomizer/native_connections.json').read_text())
for ap in connections['accessPoints']:
 dynamic=[(0x10000+ap['exit']['DoorPtr'],12),(0x70008+ap['room']['RoomPtr'],1)]+[(0x70000+a,2) for a in ap['room'].get('songs',[])]
 for address,size in dynamic:
  assert all(address+size<=a or address>=a+n for a,_,n in spans), 'World patch overlaps boss routing: '+ap['name']
from world_rom_assets import build as build_rom_assets
build_rom_assets(root,metadata,patches,spans,data,audit)
lines=['/* Generated metadata; room bytes are reconstructed from the validated ROM. */',
       f'static uint8_t world_patch_bytes[{len(data)}];',
       'static const struct {uint32_t address,offset,size;} world_spans[]={']
lines+=['{%d,%d,%d},'%s for s in spans];lines+=['};','static const struct {uint16_t first,count;} world_patches[]={']
lines+=['{%d,%d},'%p for p in patches];lines+=['};',f'#define WORLD_BACKUP_SIZE {len(data)}',f'#define WORLD_VALID_MASK UINT64_C({(1<<len(patches))-1})']
lines += [f'#define WORLD_{p["nativeBehavior"]} (UINT64_C(1)<<{p["id"]})' for p in metadata if 'nativeBehavior' in p]
indicator_id=next(p['id'] for p in metadata if p['name']=='door_indicators_plms.ips')
lines.append(f'#define WORLD_DOOR_INDICATORS (UINT64_C(1)<<{indicator_id})')
catalog=dict(schema=1,upstream=audit['upstream'],patches=metadata,requiresNativeCode=sorted(code))
catalog['sha256']=hashlib.sha256(json.dumps(catalog,sort_keys=True,separators=(',',':')).encode()).hexdigest()
lines.append('static const char world_catalog_sha256[]="'+catalog['sha256']+'";')
compatible=[catalog['sha256']]
for old in history:
 # Existing patch IDs/data are append-only. Changing them requires an explicit
 # versioned migration, never a silent reinterpretation of an older save.
 for entry in old['patches']:
  assert metadata[entry['id']]==entry, 'Historical world data changed: '+entry['name']
 if old['sha256'] not in compatible:compatible.append(old['sha256'])
lines.append('static const char *const world_compatible_catalogs[]={'+','.join('"'+h+'"' for h in compatible)+'};')

(root/'Native/sm_world_data.inc').write_text('\n'.join(lines)+'\n')
(root/'Randomizer/native_world_data.json').write_text(json.dumps(catalog,indent=2)+'\n')
ilines=['/* Generated, immutable indicator location IDs; see world catalog. */',f'#define INDICATOR_PATCH_MASK (UINT64_C(1)<<{indicator_id})',
        'static const struct {uint32_t address;uint16_t room;uint8_t x,y;uint16_t argument;uint8_t direction;} indicator_locations[]={']
for loc in indicators['locations']:
 if loc['romReplacements']:
  assert len(loc['romReplacements'])==1
  address,entry=next(iter(loc['romReplacements'].items()));room=0
 else:
  address=0;room=loc['additionalPlms']['room'];entry,=loc['additionalPlms']['plm_bytes_list']
 assert len(entry)==6
 direction=((loc['vanillaPlm']-0xfbb0)//6)%4
 ilines.append('{%d,%d,%d,%d,%d,%d},'%(int(address),room,entry[2],entry[3],entry[4]|entry[5]<<8,direction))
ilines.append('};')
(root/'Native/sm_indicators.inc').write_text('\n'.join(ilines)+'\n')
print(len(patches),'patch IDs (including',len(native_code),'native-only),',len(spans),'data spans,',len(data),'backup bytes; instruction bytes excluded:',', '.join(sorted(code)))
