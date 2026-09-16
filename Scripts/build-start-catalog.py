#!/usr/bin/env python3
"""Generate immutable native start/load/save-PLM descriptors from the audit."""
from pathlib import Path
import json,sys
root=Path(__file__).resolve().parents[1]
audit=json.loads((root/'Docs/Randomizer/FullOptions/Starts/dependencies.json').read_text())
catalog=json.loads((root/'Randomizer/native_world_data.json').read_text());ids={p['name']:p['id'] for p in catalog['patches']}
lines=['/* Pinned VARIA start records, generated from Starts/dependencies.json. */','static const struct {uint16_t spawn,save_room;uint8_t save_x,save_y,save_index,door_count,doors[3];int save_patch;uint8_t area_only;} starts[]={']
for s in audit['starts']:
 info=s['start'];save=info.get('save');plm=audit['patches'][save]['additionalPlm'] if save else None;b=plm['plm_bytes_list'][0] if plm else [0]*6;doors=info.get('doors',[])
 lines.append('{%d,%d,%d,%d,%d,%d,{%s},%d,%d},'%(info['spawn'],plm['room'] if plm else 0,b[2],b[3],b[4],len(doors),','.join(map(str,doors+[0]*(3-len(doors)))),ids[save] if save else -1,info.get('areaMode',False)))
sys.path.insert(0,str(root/'Randomizer/upstream'))
from logic.logic import Logic
Logic.factory('vanilla')
from graph.graph_utils import getAccessPoint
from utils.doorsmanager import DoorsManager
base=[0x10];DoorsManager.getBlueDoors(base);base=sorted(set(base))
entries=[]
for s in audit['starts']:
 info=getAccessPoint(s['name']).Start
 assert info==s['start'],s['name']
 entries.append(dict(name=s['name'],spawn=info['spawn'],areaOnly=bool(info.get('areaMode')),opened=sorted(set(base+info.get('doors',[])))))
initial=dict(schema=1,base=base,starts=entries)
(root/'Randomizer/native_initial_doors.json').write_text(json.dumps(initial,indent=2)+'\n')
lines+=['};','static const uint8_t initial_base_doors[]={'+','.join(map(str,base))+'};']
(root/'Native/sm_start.inc').write_text('\n'.join(lines)+'\n')
ui=['// Generated complete initial-door lists from pinned VARIA.',
    'static const struct {int32 Spawn;bool AreaOnly;int32 Count;uint8 Doors[16];} NativeInitialDoorSpecs[]={']
for e in entries:
 assert len(e['opened'])<=16
 ui.append('{%d,%s,%d,{%s}},'%(e['spawn'],'true' if e['areaOnly'] else 'false',len(e['opened']),','.join(map(str,e['opened']))))
ui.append('};')
(root/'Unreal/Source/SMUnreal/SMNativeInitialDoors.inl').write_text('\n'.join(ui)+'\n')
