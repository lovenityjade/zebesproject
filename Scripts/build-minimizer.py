#!/usr/bin/env python3
"""Compile the pinned mixed area/boss domain without changing published domains."""
import hashlib,json,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];up=root/'Randomizer/upstream';sys.path.insert(0,str(up))
from logic.logic import Logic
from graph.graph import AccessGraph
from graph.graph_utils import GraphUtils,vanillaTransitions,vanillaBossesTransitions,graphAreas
from rom.flavor import RomFlavor
from rom.enemies_objectives_data import enemies_objectives_data
Logic.factory('vanilla');RomFlavor.factory(str(up))
area=json.loads((root/'Randomizer/native_areas.json').read_text())
boss=json.loads((root/'Randomizer/native_connections.json').read_text())
names=[a['name'] for a in area['accessPoints']]+[a['name'] for a in boss['accessPoints']]
assert len(names)==len(set(names))==40
graph=AccessGraph(Logic.accessPoints(),vanillaTransitions+vanillaBossesTransitions)
GraphUtils.getDoorConnections(graph,True,True,False)
aps=[graph.accessPoints[n] for n in names]
records=[dict(id=i,name=a.Name,graphArea=a.GraphArea,room=a.RoomInfo,entry=a.EntryInfo,exit=a.ExitInfo) for i,a in enumerate(aps)]
matrix=[]
for i,src in enumerate(aps):
 row=[]
 for j,dst in enumerate(aps):
  g=AccessGraph(Logic.accessPoints(),[(src.Name,dst.Name)])
  actual=GraphUtils.getDoorConnections(g,True,True,False)
  c,=[c for c in actual if c['transition'][0].Name==src.Name]
  assert c['transition'][1].Name==dst.Name
  assert c.get('exitAsm') in (None,'door_transition_kraid_exit_fix','door_transition_boss_exit_fix')
  assert isinstance(c['doorAsmPtr'],int)
  row.append(dict(source=i,destination=j,room=c['RoomPtr'],door=c['DoorPtr'],direction=c['direction'],bitFlag=c['bitFlag'],cap=c['cap'],screen=c['screen'],asm=c['doorAsmPtr'],exitAsm=c.get('exitAsm'),distance=c['distanceToSpawn'],incompatible='SamusX' in c,x=dst.EntryInfo['SamusX'],y=dst.EntryInfo['SamusY']))
 matrix.append(row)
access=RomFlavor.patchAccess;patches={};plms={}
for ap in aps[32:]:
 name='Blinking['+ap.Name+']'
 if name in access.getDictPatches():patches[name]=access.getDictPatches()[name]
 if name in access.getAdditionalPLMs():plms[name]=access.getAdditionalPLMs()[name]
assert all(0x70000<=a<0x80000 or 0x100000<=a<0x110000 for patch in patches.values() for a in patch)
geometry=sorted(json.loads((root/'Randomizer/native_locations.json').read_text())['locations'],key=lambda p:p['address'])
byaddr={l.Address:l for l in Logic.locations() if not l.isBoss()}
locations=[dict(name=p['name'],address=p['address'],region=graphAreas.index(byaddr[p['address']].GraphArea),postBoss=byaddr[p['address']].SolveArea.endswith(' Boss')) for p in geometry]
catalog=dict(schema=1,abiSize=152,areaCatalogSha256=area['sha256'],bossCatalogSha256=boss['sha256'],accessPoints=records,connections=matrix,
 regions=graphAreas,locations=locations,blinkPatches=patches,roomPlms=plms,
 enemyRegionTotals=[[e['area_count'].get(r,0) for r in graphAreas] for e in enemies_objectives_data.values()],
 bossTileTotals=[{'Kraid':5,'WreckedShip':1,'EastMaridia':5,'LowerNorfair':3}.get(r,0) for r in graphAreas])
catalog['sha256']=hashlib.sha256(json.dumps(catalog,sort_keys=True,separators=(',',':')).encode()).hexdigest()
p=root/'Randomizer/native_minimizer.json'
if p.exists():assert json.loads(p.read_text())==json.loads(json.dumps(catalog)),'Version mixed-domain identities before changing them'
else:p.write_text(json.dumps(catalog,indent=2)+'\n')
lines=['/* Mixed area/boss domain; generated from the pinned actual door writer. */',f'static const char minimizer_catalog[]="{catalog["sha256"]}";',
 'static const struct {uint16_t door,room;uint8_t song_count,song;uint16_t songs[3];} minimizer_aps[40]={']
for a in aps:
 songs=a.RoomInfo.get('songs',[]);assert len(songs)<=3
 lines.append('{%d,%d,%d,%d,{%s}},'%(a.ExitInfo['DoorPtr'],a.RoomInfo['RoomPtr'],len(songs),a.EntryInfo.get('song',0),','.join(map(str,songs+[0]*(3-len(songs))))))
lines+=['};','typedef struct {uint8_t direction,flag,cap_x,cap_y,screen_x,screen_y,incompatible,exit_fix;uint16_t distance,asm_ptr,x,y;} MixedConnection;','static const MixedConnection minimizer_matrix[40][40]={']
for row in matrix:
 lines.append('{')
 for c in row:
  vals=[c['direction'],c['bitFlag'],*c['cap'],*c['screen'],int(c['incompatible']),{None:0,'door_transition_kraid_exit_fix':1,'door_transition_boss_exit_fix':2}[c['exitAsm']],c['distance'],c['asm'],c['x'],c['y']]
  lines.append('{'+','.join(map(str,vals))+'},')
 lines.append('},')
lines+=['};','static const struct {uint32_t address;uint16_t size,offset;} minimizer_spans[]={']
data=[]
for patch in patches.values():
 for a,v in patch.items():lines.append('{%d,%d,%d},'%(a,len(v),len(data)));data+=v
lines+=['};','static const uint8_t minimizer_data[]={'+','.join(map(str,data))+'};', '#define MINIMIZER_DATA_SIZE '+str(len(data))]
lines+=['static const struct {uint16_t room,state;uint8_t entry[6];} minimizer_plms[]={']
for plm in plms.values():
 for entry in plm['plm_bytes_list']:lines.append('{%d,%d,{%s}},'%(plm['room'],plm.get('state',0),','.join(map(str,entry))))
lines+=['};']
lines+=['static const uint8_t minimizer_room_areas[40]={'+','.join(str(a.RoomInfo['area']) for a in aps)+'};',
 'static const struct {uint8_t region,post_boss;} minimizer_locations[100]={'+','.join('{%d,%d}'%(l['region'],l['postBoss']) for l in locations)+'};',
 'static const uint8_t minimizer_boss_totals[12]={'+','.join(map(str,catalog['bossTileTotals']))+'};',
 'static const uint8_t minimizer_enemy_totals[6][12]={'+','.join('{'+','.join(map(str,row))+'}' for row in catalog['enemyRegionTotals'])+'};']
from rom.map import AreaMap
from graph.graph_utils import gameAreas
boss_rooms={'Kraid Room','Varia Suit Room',"Phantoon's Room","Draygon's Room",'Space Jump Room',"Ridley's Room",'Ridley Tank Room'}
boss_tiles=[];found=set()
for a,name in enumerate(gameAreas):
 slug='wrecked_ship' if name=='WreckedShip' else name.lower()
 for region,rooms in json.loads((up/f'tools/map/graph_area/normal_{slug}.json').read_text()).items():
  for name,coords in rooms.items():
   if name not in boss_rooms:continue
   found.add(name)
   for x,y in coords:
    byte,mask=AreaMap().getByteIndexMask(x,y);boss_tiles.append((a,byte,mask))
assert found==boss_rooms and len(boss_tiles)==14
lines+=['static const struct {uint8_t area,byte,mask;} minimizer_boss_tiles[]={'+','.join('{%d,%d,%d}'%t for t in boss_tiles)+'};']
(root/'Native/sm_minimizer_routing.inc').write_text('\n'.join(lines)+'\n')
ue='// Generated from the pinned native mixed-domain catalog.\n'
ue+=f'static const TCHAR* NativeMinimizerCatalog=TEXT("{catalog["sha256"]}");\n'
ue+='static const TCHAR* NativeMinimizerEndpoints[]={'+','.join('TEXT('+json.dumps(a['name'])+')' for a in records)+'};\n'
ue+='static const struct {const TCHAR* Name;uint8 Region;bool PostBoss;} NativeMinimizerLocations[]={'+','.join('{TEXT(%s),%d,%s}'%(json.dumps(l['name']),l['region'],'true' if l['postBoss'] else 'false') for l in locations)+'};\n'
ue+='static const uint16 NativeMinimizerEnemies[6][12]={'+','.join('{'+','.join(map(str,row))+'}' for row in catalog['enemyRegionTotals'])+'};\n'
ue+='static const uint16 NativeMinimizerBossTiles[12]={'+','.join(map(str,catalog['bossTileTotals']))+'};\n'
(root/'Unreal/Source/SMUnreal/SMNativeMinimizer.inl').write_text(ue)
out=root/'Docs/Randomizer/FullOptions/Minimizer';out.mkdir(exist_ok=True)
(out/'catalog-audit.json').write_text(json.dumps(dict(catalogSha256=catalog['sha256'],endpoints=40,descriptors=1600,blinkPatches=list(patches),roomPlms=plms,dataBytes=len(data)),indent=2)+'\n')
print('MINIMIZER_CATALOG',40,1600,len(data),catalog['sha256'])
