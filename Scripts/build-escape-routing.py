#!/usr/bin/env python3
"""Compile escape door descriptors from the pinned VARIA graph writer."""
import copy,hashlib,json,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];up=root/'Randomizer/upstream';sys.path.insert(0,str(up))
from logic.logic import Logic
from graph.graph import AccessGraph
from graph.graph_utils import GraphUtils,vanillaEscapeTransitions,vanillaEscapeAnimalsTransitions
from rom.flavor import RomFlavor
Logic.factory('vanilla');RomFlavor.factory(str(up))
aps=Logic.accessPoints();byname={a.Name:a for a in aps}
for i in range(4):
 a=copy.copy(byname['Flyway Right']);a.Name+=' '+str(i);a.ExitInfo['DoorPtr']=None
 a.ExitInfo['DoorPtrSym']='rando_escape_flyway_door_lists_door'+str(i);aps.append(a)
a=copy.copy(byname['Bomb Torizo Room Left']);a.Name+=' Animals';a.ExitInfo['DoorPtr']=None
a.ExitInfo['DoorPtrSym']='rando_escape_common_bt_door_list';aps.append(a)
names=list(dict.fromkeys(n for pair in vanillaEscapeTransitions+vanillaEscapeAnimalsTransitions for n in pair))
graph=AccessGraph(aps,vanillaEscapeTransitions+vanillaEscapeAnimalsTransitions)
GraphUtils.getDoorConnections(graph,False,False,True,True)
records=[];matrix=[]
def door(c):return RomFlavor.symbols.getAddress(c['DoorPtrSym'])&65535 if 'DoorPtrSym' in c else c['DoorPtr']
for i,name in enumerate(names):
 a=graph.accessPoints[name]
 records.append(dict(id=i,name=name,door=door(a.ExitInfo),room=a.RoomInfo['RoomPtr']))
 row=[]
 for j,target in enumerate(names):
  g=AccessGraph(list(graph.accessPoints.values()),[(name,target)])
  actual=GraphUtils.getDoorConnections(g,False,False,True,True)
  c,=[c for c in actual if c['transition'][0].Name==name]
  assert c.get('exitAsm') in (None,'rando_escape_common_setup_next_escape')
  assert isinstance(c['doorAsmPtr'],int)
  dst=c['transition'][1]
  row.append(dict(source=i,destination=j,door=door(c),room=c['RoomPtr'],direction=c['direction'],flag=c['bitFlag'],cap=c['cap'],screen=c['screen'],distance=c['distanceToSpawn'],asm=c['doorAsmPtr'],incompatible='SamusX' in c,x=dst.EntryInfo['SamusX'],y=dst.EntryInfo['SamusY'],cycle=bool(c.get('exitAsm'))))
 matrix.append(row)
catalog=dict(schema=1,abiSize=44,accessPoints=records,connections=matrix)
catalog['sha256']=hashlib.sha256(json.dumps(catalog,sort_keys=True,separators=(',',':')).encode()).hexdigest()
p=root/'Randomizer/native_escape.json'
if p.exists():assert json.loads(p.read_text())==json.loads(json.dumps(catalog)),'Version escape identities before changing them'
else:p.write_text(json.dumps(catalog,indent=2)+'\n')
lines=['/* Generated escape routing: original geometry and original native callbacks. */',f'static const char escape_catalog[]="{catalog["sha256"]}";',f'#define ESCAPE_ENDPOINTS {len(records)}',
 'static const struct {uint16_t door,room;} escape_aps[]={']
lines+=['{%d,%d},'%(a['door'],a['room']) for a in records]
lines+=['};','typedef struct {uint8_t direction,flag,cap_x,cap_y,screen_x,screen_y,incompatible,cycle;uint16_t distance,asm_ptr,x,y;} EscapeConnection;',f'static const EscapeConnection escape_matrix[{len(records)}][{len(records)}]={{']
for row in matrix:
 lines.append('{')
 for c in row:
  vals=[c['direction'],c['flag'],*c['cap'],*c['screen'],int(c['incompatible']),int(c['cycle']),c['distance'],c['asm'],c['x'],c['y']]
  lines.append('{'+','.join(map(str,vals))+'},')
 lines.append('},')
lines+=['};'];(root/'Native/sm_escape_routing.inc').write_text('\n'.join(lines)+'\n')
ue='// Generated pinned escape identities.\n'+f'static const TCHAR* NativeEscapeCatalog=TEXT("{catalog["sha256"]}");\n'
ue+='static const TCHAR* NativeEscapeEndpoints[]={'+','.join('TEXT('+json.dumps(a['name'])+')' for a in records)+'};\n'
ue+='static const uint16 NativeEscapeDoors[]={'+','.join(str(a['door']) for a in records)+'};\n'
(root/'Unreal/Source/SMUnreal/SMNativeEscape.inl').write_text(ue)
print('ESCAPE_ROUTING_CATALOG',len(records),len(records)**2,catalog['sha256'])
