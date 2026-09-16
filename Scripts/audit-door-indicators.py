#!/usr/bin/env python3
"""Reconstruct VARIA's indicator IPS as native PLM data, without running a ROM.

This audit is deliberately separate from enabling the patch. Per-seed room
insertions/replacements and native door lifecycle tests are still required.
"""
from pathlib import Path
import hashlib,json,sys
root=Path(__file__).resolve().parents[1];upstream=root/'Randomizer/upstream'
sys.path.insert(0,str(upstream))
from logic.logic import Logic
Logic.factory('vanilla')
from rom.flavor import RomFlavor
RomFlavor.factory(str(upstream))
from rom.ips import IPS_Patch
from utils.doorsmanager import DoorsManager,IndicatorAll,IndicatorFlag
blob=bytearray();records=[]
def words(*values):
 for value in values:blob.extend(value.to_bytes(2,'little'))
for color,base_draw in [('red',0xa8e7),('green',0xa827),('yellow',0xa767),('grey',0xa6a7)]:
 for direction in range(4):
  close=0xf900+len(blob)
  words(2,0xa677+direction*12,2,0xa9d7+direction*0x3c,0x8c19)
  blob.append(8)
  words(2,0xa9cb+direction*0x3c,2,0xa9bf+direction*0x3c)
  main=0xf900+len(blob)
  words(0x8a72,0xc4b1+direction*0x31,0x8a24,0xc489+direction*0x31,0x86c1,0xbd0f)
  loop=0xf900+len(blob)
  words(9,0xa9b3+direction*0x3c,9,base_draw+direction*0x30,0x8724,loop)
  records.append(dict(color=color,direction=direction,close=close,main=main))
for record in records:
 record['plm']=0xf900+len(blob)
 words(0xc7b1,record['main'],record['close'])
patch=IPS_Patch.load(str(upstream/'patches/common/ips/door_indicators_plms.ips')).toDict()
assert patch=={0x27900:list(blob)} and len(blob)==784
source=(root/'native-core/src/sm_84.c').read_text()
handlers={0x8c19:'PlmInstr_QueueSfx3_Max6',0x8a72:'PlmInstr_GotoIfDoorBitSet',
          0x8a24:'PlmInstr_SetLinkReg',0x86c1:'PlmInstr_PreInstr',0xbd0f:'PlmPreInstr_GoToLinkInstrIfShot',
          0x8724:'PlmInstr_Goto',0xc7b1:'PlmSetup_Door_Colored'}
for address,name in handlers.items():
 assert name in source,(hex(address),name)
pa=RomFlavor.patchAccess
locations=[]
for name,plm in DoorsManager.getIndicatorPLMs(IndicatorAll).items():
 key=f'Indicator[{name}]';door=DoorsManager.doors[name]
 definition=pa.getDictPatches().get(key)
 extra=pa.getAdditionalPLMs().get(key)
 assert bool(definition)!=bool(extra),name
 locations.append(dict(name=name,vanillaPlm=plm,flags=int(door.indicator),
                       romReplacements=definition,additionalPlms=extra))
out=root/'Docs/Randomizer/FullOptions/DoorIndicators';out.mkdir(exist_ok=True)
(out/'audit.json').write_text(json.dumps(dict(schema=1,upstream='72ec1f30b700442d0c30aad6c43801bd64f1e2a5',
 ipsSha256=hashlib.sha256(blob).hexdigest(),dataBytes=len(blob),nativeHandlers=handlers,records=records,
 locations=locations,standard=list(DoorsManager.getIndicatorPLMs(IndicatorFlag.Standard)),
 scope='Exact IPS reconstruction and native handler existence; room insertion, state behavior and rendering remain unimplemented'),indent=2)+'\n')
print('INDICATOR_DATA_AUDIT',len(blob),'bytes,',len(records),'PLMs,',len(locations),'locations')
