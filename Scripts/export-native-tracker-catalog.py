#!/usr/bin/env python3
"""Read original ROM geometry and audited identities; never runs the game."""
import ast
import hashlib
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
tree=ast.parse((ROOT/'Randomizer/upstream/tools/rooms.py').read_text())
rooms=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='rooms' for t in n.targets))
by_name={r['Name']:r for r in rooms}
rom=(ROOT/'roms/Super Metroid (Japan, USA) (En,Ja).sfc').read_bytes()
assert hashlib.sha1(rom).hexdigest()=='da957f0d63d14cb441d215462904c4fa8519c613'
entries=[]
for r in json.loads((ROOT/'Docs/References/PopTracker/inventaires/native-crosswalk.json').read_text()):
 loc=r['native'];addr=loc['Address'];name=loc['Room']
 name={'Billy Mays Room':'Blue Brinstar Double Missile Room'}.get(name,name)
 matches=[room for room in rooms if room['Name']==name or room['Name'].split(' [')[0]==name]
 assert len(matches)==1,(name,[room['Name'] for room in matches])
 room=matches[0];pc=room['Address'];bit=int.from_bytes(rom[addr+4:addr+6],'little')&255
 assert bit==loc['Id'],(loc['Name'],bit,loc['Id'])
 assert rom[addr+2]<rom[pc+4]*16 and rom[addr+3]<rom[pc+5]*16, loc['Name']
 entries.append(dict(name=loc['Name'],address=addr,collectionBit=bit,area=rom[pc+1],room=name,roomPointer=pc&65535,
  roomHeaderPc=pc,roomMapX=rom[pc+2],roomMapY=rom[pc+3],roomWidth=rom[pc+4],roomHeight=rom[pc+5],
  plmX=rom[addr+2],plmY=rom[addr+3],mapX=rom[pc+2]+(rom[addr+2]>>4),mapY=rom[pc+3]+(rom[addr+3]>>4)+1,
  visibility=loc['Visibility'],apId=r['apId'],popTrackerPath=r['trackerPath']))
entries.sort(key=lambda x:x['address']);assert len(entries)==100 and len({e['collectionBit'] for e in entries})==100
(ROOT/'Randomizer/native_locations.json').write_text(json.dumps(dict(schema=1,romSha1=hashlib.sha1(rom).hexdigest(),locations=entries),indent=2)+'\n')
print('Extracted 100 native locations and unique collection bits from the original ROM and audited crosswalk.')
