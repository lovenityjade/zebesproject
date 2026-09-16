#!/usr/bin/env python3
"""Compile pinned boss door descriptors and every compatible/incompatible pair.
No seed generation, ROM execution, or changes to the upstream checkout.
"""
import hashlib,json,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];sys.path.insert(0,str(root/'Randomizer/upstream'))
from logic.logic import Logic
Logic.factory('vanilla')
from graph.graph import AccessGraph
from graph.graph_utils import GraphUtils,vanillaBossesTransitions
graph=AccessGraph(Logic.accessPoints(),vanillaBossesTransitions)
GraphUtils.getDoorConnections(graph,False,True,False)
aps=sorted((ap for ap in graph.accessPoints.values() if ap.Boss),key=lambda ap:ap.Name)
assert len(aps)==8
records=[]
for i,ap in enumerate(aps):
 records.append(dict(id=i,name=ap.Name,inside=ap.Name.endswith('In'),room=ap.RoomInfo,entry=ap.EntryInfo,exit=ap.ExitInfo))
connections=[]
for i,src in enumerate(aps):
 row=[]
 for j,dst in enumerate(aps):
  if src.Name.endswith('In')==dst.Name.endswith('In'):
   row.append(None);continue
  c=dict(source=i,destination=j,room=dst.RoomInfo['RoomPtr'],door=src.ExitInfo['DoorPtr'],
         direction=GraphUtils.getDirection(src,dst),bitFlag=GraphUtils.getBitFlag(src.RoomInfo['area'],dst.RoomInfo['area'],dst.EntryInfo['bitFlag']),
         cap=dst.EntryInfo['cap'],screen=dst.EntryInfo['screen'],asm=dst.EntryInfo['doorAsmPtr'],
         exitAsm=src.ExitInfo.get('exitAsm'),distance=dst.EntryInfo['distanceToSpawn'])
  c['incompatible']=c['direction']!=src.ExitInfo['direction']
  c['x']=dst.EntryInfo['SamusX'];c['y']=dst.EntryInfo['SamusY']
  if c['incompatible']:
   c['distance']=0
   if dst.Name=='RidleyRoomIn':c['screen']=(0,1)
  assert isinstance(c['asm'],int) and c['asm'] in (0,0xe1fe,0xe3d9)
  assert c['exitAsm'] in (None,'door_transition_kraid_exit_fix','door_transition_boss_exit_fix')
  # Compare the descriptor against VARIA's actual authoritative writer input.
  pair=AccessGraph(Logic.accessPoints(),[(src.Name,dst.Name)])
  actual=GraphUtils.getDoorConnections(pair,False,True,False)
  a=next(x for x in actual if x['transition'][0].Name==src.Name and x['transition'][1].Name==dst.Name)
  for key,other in [('room','RoomPtr'),('door','DoorPtr'),('direction','direction'),('bitFlag','bitFlag'),('cap','cap'),('screen','screen'),('asm','doorAsmPtr'),('distance','distanceToSpawn')]:
   assert c[key]==a[other],(src.Name,dst.Name,key,c[key],a[other])
  row.append(c)
 connections.append(row)
from rom.flavor import RomFlavor
RomFlavor.factory(str(root/'Randomizer/upstream'))
from rom.rom_patches import getPatchSet
boss=getPatchSet('boss','vanilla')
room_plms={name:RomFlavor.patchAccess.getAdditionalPLMs()[name] for name in boss['plms']}
assert list(room_plms)==['WS_Save_Blinking_Door']
assert room_plms['WS_Save_Blinking_Door']==dict(room=0xcaf6,state=0xcb08,plm_bytes_list=[[0x42,0xc8,0x4e,0x36,0x62,0x0c]])
catalog=dict(roomPlms=room_plms,patches=boss['ips'],schema=1,upstream='72ec1f30b700442d0c30aad6c43801bd64f1e2a5',accessPoints=records,connections=connections,vanillaPairs=vanillaBossesTransitions)
catalog['sha256']=hashlib.sha256(json.dumps(catalog,sort_keys=True,separators=(',',':')).encode()).hexdigest()
# Published boss routing IDs/semantics cannot silently change. A future
# extension needs a new contract plus explicit historical-catalog handling.
for path in (root/'Randomizer/connection_catalog_history').glob('*.json'):
 assert json.loads(path.read_text())==json.loads(json.dumps(catalog)), 'Published boss catalog changed: '+path.name
lines=['/* Generated from pinned VARIA graph/door writer. Boss domain only. */',
       'static const char connections_catalog[]="'+catalog['sha256']+'";',
       'static const struct {uint16_t door,room;uint8_t inside,song_count,song;uint16_t songs[3];} connection_aps[]={']
for ap in aps:
 songs=ap.RoomInfo.get('songs',[]);assert len(songs)<=3
 lines.append('{%d,%d,%d,%d,%d,{%s}},'%(ap.ExitInfo['DoorPtr'],ap.RoomInfo['RoomPtr'],ap.Name.endswith('In'),len(songs),ap.EntryInfo.get('song',0),','.join(map(str,songs+[0]*(3-len(songs))))))
lines+=['};','typedef struct {uint8_t direction,flag,cap_x,cap_y,screen_x,screen_y,incompatible,exit_fix;uint16_t distance,asm_ptr,x,y;} Connection;','static const Connection connection_matrix[8][8]={']
for row in connections:
 lines.append('{')
 for c in row:
  if c is None:lines.append('{0},');continue
  fix={None:0,'door_transition_kraid_exit_fix':1,'door_transition_boss_exit_fix':2}[c['exitAsm']]
  lines.append('{%s},'%','.join(map(str,[c['direction'],c['bitFlag'],*c['cap'],*c['screen'],int(c['incompatible']),fix,c['distance'],c['asm'],c['x'],c['y']])))
 lines.append('},')
lines.append('};')
p=room_plms['WS_Save_Blinking_Door'];entry,=p['plm_bytes_list']
lines.append('static const struct {uint16_t room,state;uint8_t entry[6];} boss_save_plm={%d,%d,{%s}};'%(p['room'],p['state'],','.join(map(str,entry))))
(root/'Native/sm_connections.inc').write_text('\n'.join(lines)+'\n')
(root/'Randomizer/native_connections.json').write_text(json.dumps(catalog,indent=2)+'\n')
out=root/'Docs/Randomizer/FullOptions/Connections';out.mkdir(exist_ok=True)
(out/'audit.json').write_text(json.dumps(catalog,indent=2)+'\n')
print('BOSS_CONNECTION_CATALOG',len(records),'access points,',sum(c is not None for row in connections for c in row),'verified directed combinations')
