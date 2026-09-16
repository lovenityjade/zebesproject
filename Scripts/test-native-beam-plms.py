#!/usr/bin/env python3
"""Actual PLM setup/draw/open/save behavior for every beam-door orientation."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-door-colors.py').read_text().split('cases=[]')[0]
source=source.replace("root/'door-colors/native'","root/'door-colors/plms'")
exec(compile(source,'beam-plm-fixture','exec'))
values=(C.c_uint8*58)(*[5 if d['canRandom'] else 0 for d in locations])
assert l.sm_doors_configure(0,values,58,catalog['sha256'].encode());activate(0);assert select(items,100,m['sha256'].encode())
spawn=routine('SpawnRoomPLM',None,C.c_uint16);handler=routine('PlmHandler_Async',C.c_int)
results=[]
for record in catalog['records']:
 C.memmove(ram,baseline,len(baseline));C.memset(ram+0x1c37,0,80);C.memset(ram+0xd8b0,0,64);put(0x1c23,0x8000)
 plm=record['plm'];direction=record['direction'];entry=plm.to_bytes(2,'little')+bytes([6,6,5,0])
 C.memmove(ram+0x12,entry,6);spawn(0x12);index=39
 assert handler()==0
 routine('LoadXrayBlocks',None)()
 if record['indicator']:
  blue=0xa9b3+direction*0x3c;beam=next(d['draw'] for d in catalog['records'] if not d['indicator'] and d['color']==record['color'] and d['direction']==direction)
  phases=[word(0xde6c+index*2)]
  for frame in range(18):assert handler()==0;phases.append(word(0xde6c+index*2))
  assert phases==[blue]*9+[beam]*9+[blue],record
  put(0x1d77+index*2,0x100);assert handler()==0
  assert not read(0xd8b0,1)[0]&32
 else:
  assert word(0xde6c+index*2)==record['draw'],record
  put(0x1d77+index*2,0x500);assert handler()==0
  assert not read(0xd8b0,1)[0]&32
  shot={'wave':1,'ice':2,'spazer':4,'plasma':8}[record['color']]
  put(0x1d77+index*2,shot);assert handler()==0
  assert read(0xd8b0,1)[0]&32,(record,word(0x1d27+index*2))
 # Both types use original blue-door opening animation, and disappear.
 for frame in range(120):assert handler()==0
 assert word(0x1c37+index*2)==0,record
 C.memmove(ram+0xd8b0,bytes([32]),1);C.memmove(ram+0x12,entry,6);spawn(0x12)
 assert handler()==0 and word(0xde6c+index*2)==0xa9b3+direction*0x3c
 assert handler()==0 and word(0x1c37+index*2)==0
 results.append(record)
# Red doors use the new missile-only pre-instruction and exactly one hit.
for direction in range(4):
 C.memmove(ram,baseline,len(baseline));C.memset(ram+0x1c37,0,80);C.memset(ram+0xd8b0,0,64);put(0x1c23,0x8000)
 entry=(0xc88a+direction*6).to_bytes(2,'little')+bytes([6,6,5,0]);C.memmove(ram+0x12,entry,6);spawn(0x12);assert handler()==0
 assert word(0x1cd7+78)==0x8560
 put(0x1d77+78,0x200);assert handler()==0;assert not read(0xd8b0,1)[0]&32
 put(0x1d77+78,0x100);assert handler()==0;assert read(0xd8b0,1)[0]&32
# Permanently grey doors must stay sealed even when normal room/boss/event
# conditions would open an ordinary grey door.
for direction in range(4):
 C.memmove(ram,baseline,len(baseline));C.memset(ram+0x1c37,0,80);C.memset(ram+0xd8b0,0,64);put(0x1c23,0x8000)
 C.memset(ram+0xd820,0xff,16)
 entry=(0xc842+direction*6).to_bytes(2,'little')+bytes([6,6,5,0x90]);C.memmove(ram+0x12,entry,6);spawn(0x12)
 for frame in range(180):
  put(0x1d77+78,0x1fff);assert handler()==0
 assert not read(0xd8b0,1)[0]&32 and word(0x1c37+78)==0xc842+direction*6
C.memmove(ram+0xd8b0,bytes([32]),1)
routine('SaveToSram',None,C.c_uint16)(0);C.memset(ram+0xd8b0,0,64);assert routine('LoadFromSram',C.c_uint8,C.c_uint16)(0)==0
assert read(0xd8b0,1)[0]&32
# A beam indicator cannot be configured or activated without its native
# colored-door data. Preparing a disabled selector must not permit bad PLMs.
class Indicator(C.Structure):_fields_=[('location',C.c_uint16),('plm',C.c_uint16)]
l.sm_start_configure_world.argtypes=[C.c_int,C.c_int,C.POINTER(C.c_uint8),C.c_int,C.c_char_p,C.POINTER(Indicator),C.c_int]
world=json.loads((root/'Randomizer/native_world_data.json').read_text());patch_id=next(p['id'] for p in world['patches'] if p['name']=='door_indicators_plms.ips')
patch=(C.c_uint8*1)(patch_id);indicator=(Indicator*1)(Indicator(0,0xf611))
assert not l.sm_start_configure_world(1,0,patch,1,world['sha256'].encode(),indicator,1)
assert l.sm_doors_configure(1,values,58,catalog['sha256'].encode())
assert l.sm_start_configure_world(1,0,patch,1,world['sha256'].encode(),indicator,1)
assert l.sm_doors_configure(1,None,0,catalog['sha256'].encode())
assert not routine('sm_start_activate',C.c_int,C.c_int,C.c_int)(1,1)
assert l.sm_cpu_opcodes()==0;l.sm_shutdown();assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(out/'verification.json').write_text(json.dumps(dict(plms=results,redOrientations=4,sealedGreyOrientations=4,beamDependencyRejected=True,openedBitSaveReload=True,xraySafe=True,cpu=0,sourceRomUnchanged=True,scope=__doc__),indent=2))
print('NATIVE_BEAM_PLMS_PASS',len(results),4,flush=True)
