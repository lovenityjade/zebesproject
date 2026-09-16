#!/usr/bin/env python3
"""Compile VARIA's authored exploration ownership, slopes and relocated portals.

Read source patches in memory only. The runtime retains original map graphics.
"""
import ast,hashlib,json,re,sys
from collections import Counter
from pathlib import Path
root=Path(__file__).resolve().parents[1];up=root/'Randomizer/upstream'
sys.path.insert(0,str(up))
from logic.logic import Logic
Logic.factory('vanilla')
from graph.graph_utils import gameAreas,graphAreas,getAccessPoint
from rom.rom import snes_to_pc
from rom.ips import IPS_Patch
from rom.map import AreaMap
rom=(root/'roms/Super Metroid (Japan, USA) (En,Ja).sfc').read_bytes()
assert hashlib.sha256(rom).hexdigest()=='12b77c4bc9c1832cee8881244659065ee1d84c70c3d29e6eaf92e6798cc2ca72'
def word(data,offset):return int.from_bytes(data[offset:offset+2],'little')
areas=[*gameAreas,'Ceres'];amap=AreaMap();owned={};sources=[]
def overlay(prefix):
    for a,name in enumerate(areas):
        slug='wrecked_ship' if name=='WreckedShip' else name.lower()
        path=up/f'tools/map/graph_area/{prefix}_{slug}.json'
        if not path.exists():continue
        sources.append(path)
        for region,rooms in json.loads(path.read_text()).items():
            for room,coords in rooms.items():
                if room=='__unexplorable__':continue
                for x,y in coords:owned[a,x,y]=graphAreas.index(region)
layouts={};overlay('normal');layouts['area_rando']=dict(owned)
overlay('alt');layouts['vanilla_layout']=dict(owned)
assert len(owned)==1262 and layouts['area_rando'].keys()==owned.keys()
for name,tiles in layouts.items():
    assert {graphAreas[i]:n for i,n in Counter(tiles.values()).items()}==Logic.map_tilecount[name]
# Check exact source map words, not native tile numbers: VARIA rearranges gfx.
patched=rom
for name in ['map_data','map_data_area']:
    path=up/f'patches/vanilla/ips/{name}.ips';sources.append(path)
    patched=IPS_Patch.load(path).apply(patched)
alt=up/'patches/vanilla/ips/map_data_area_alt.ips';sources.append(alt)
vanilla=IPS_Patch.load(alt).apply(patched)
def address(a,x,y):
    ptr=snes_to_pc(0x82964a)+a*3
    return snes_to_pc(int.from_bytes(rom[ptr:ptr+3],'little'))+2*((x//32)*1024+y*32+x%32)
slopes=[];missing=[];tiles=[]
for (a,x,y),owner in sorted(owned.items()):
    byte,mask=amap.getByteIndexMask(x,y);addr=address(a,x,y)
    native=word(rom,addr);source=word(patched,addr)
    assert (source&1023)!=31 and (word(vanilla,addr)&1023)!=31
    assert ((source&1023)==17)==((word(vanilla,addr)&1023)==17)
    record=dict(area=a,x=x,y=y,byte=byte,mask=mask,owners=[owner,layouts['area_rando'][a,x,y]])
    tiles.append(record)
    if source&1023==17:slopes.append(dict(area=a,byte=byte,mask=mask,x=x,y=y))
    if native&1023==31:missing.append(dict(address=addr,original=native,source=source,native=source&~0x1c00|0xc00))
assert missing==[dict(address=0x1a820e,original=31,source=0xdc25,native=0xcc25)]
assert len(slopes)==8
for a in range(7):
    for x in range(64):
        for y in range(32):
            if word(patched,address(a,x,y))&1023==17:assert (a,x,y) in owned
# The one added shape must use exactly the original game's map tile pixels.
gfx=up/'patches/common/src/map/pause.gfx';sources.append(gfx)
assert rom[snes_to_pc(0xb68000)+0x25*32:snes_to_pc(0xb68000)+0x26*32]==gfx.read_bytes()[0x25*32:0x26*32]
catalog=json.loads((root/'Randomizer/native_areas.json').read_text());portals=[]
for entry in catalog['accessPoints']:
    ap=getAccessPoint(entry['name']);info=Logic.map_tiles.areaAccessPointsInGameDisplay.get(entry['name'])
    data=dict(id=entry['id'],name=entry['name'],roomArea=ap.RoomInfo['area'],relocated=info is not None)
    if info:
        x,y=info['coords'];byte,mask=amap.getByteIndexMask(x,y)
        data.update(area=areas.index(info['area']),byte=byte,mask=mask,x=x,y=y,coversTile=bool(info.get('coversTile')))
        assert ((data['area'],x,y) in owned)==data['coversTile']
    portals.append(data)
# Original SRAM compression must preserve every countable/portal byte.
packed={}
for a in range(6):
    count=rom[snes_to_pc(0x818131)+a]
    ptr=word(rom,snes_to_pc(0x8182d6)+a*2)
    packed[a]=set(rom[snes_to_pc(0x810000|ptr):snes_to_pc(0x810000|ptr)+count])
unpacked=[p for p in tiles+[p for p in portals if p['relocated']] if p['area']<6 and p['byte'] not in packed[p['area']]]
assert len(unpacked)==1 and unpacked[0]['name']=='East Tunnel Top Right'
assert (unpacked[0]['area'],unpacked[0]['byte'],unpacked[0]['mask'])==(1,201,128)
assert max(word(rom,snes_to_pc(0x818138)+a*2)+rom[snes_to_pc(0x818131)+a] for a in range(6))==327
# Runtime's ZME1 extension preserves this one otherwise-unpacked portal bit.
# Source room-state region bytes differ for nine rooms between layouts.
roomtree=ast.parse((up/'tools/rooms.py').read_text())
rooms=next(ast.literal_eval(n.value) for n in roomtree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='rooms' for t in n.targets))
roomids={}
for layout in ['vanilla_layout','area_rando']:
    path=up/f'patches/vanilla/src/area_ids_{layout}.asm';sources.append(path)
    table={}
    for name,address,region,unused in re.findall(r';;; ([^\n]+)\norg \$([0-9a-f]+)\n\s*db \$([0-9a-f]+), \$([0-9a-f]+)',path.read_text()):
        assert name not in table or table[name]==int(region,16)
        table[name]=int(region,16)
    assert len(table)==261 and set(table)=={r['Name'] for r in rooms}
    roomids[layout]=table
room_records=[dict(name=r['Name'],room=r['Address']&65535,owners=[roomids[l][r['Name']] for l in ['vanilla_layout','area_rando']]) for r in rooms]
assert sum(r['owners'][0]!=r['owners'][1] for r in room_records)==9
audit=dict(schema=1,layouts={name:Logic.map_tilecount[name] for name in layouts},regions=graphAreas,areas=areas,
    tiles=tiles,slopes=slopes,portals=portals,rooms=room_records,mapEdits=missing,unpersistedBytes=unpacked,
    sources={str(p.relative_to(up)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources})
out=root/'Docs/Randomizer/FullOptions/MapExploration';out.mkdir(exist_ok=True)
(out/'audit.json').write_text(json.dumps(audit,indent=2)+'\n')
lines=['/* Generated by build-map-exploration.py; original VARIA exploration ownership. */',
 'static const struct {uint8_t area,byte,mask,owner[2];} exploration_tiles[]={']
lines+=['{%d,%d,%d,{%d,%d}},'%(p['area'],p['byte'],p['mask'],*p['owners']) for p in tiles]
lines+=['};','static const struct {uint8_t area,byte,mask;} exploration_slopes[]={']
lines+=['{%d,%d,%d},'%(p['area'],p['byte'],p['mask']) for p in slopes]
lines+=['};','static const struct {uint8_t room_area,relocated,area,byte,mask;} exploration_portals[]={']
lines+=['{%d,%d,%d,%d,%d},'%(p['roomArea'],p['relocated'],p.get('area',0),p.get('byte',0),p.get('mask',0)) for p in portals]
lines+=['};','static const uint16_t exploration_totals[2][12]={']
for name in ['vanilla_layout','area_rando']:
    lines+=['{'+','.join(str(Logic.map_tilecount[name][r]) if r not in ['Ceres','Tourian'] else '0' for r in graphAreas)+'},']
lines+=['};']
lines+=['static const struct {uint16_t room;uint8_t owner[2];} exploration_rooms[]={']
lines+=['{%d,{%d,%d}},'%(r['room'],*r['owners']) for r in room_records]
lines+=['};']
(root/'Native/sm_map_exploration.inc').write_text('\n'.join(lines)+'\n')
print('MAP_EXPLORATION',len(tiles),'tiles',len(slopes),'slopes',sum(p['relocated'] for p in portals),'relocated portals',len(unpacked),'unpersisted tile/portal records')
