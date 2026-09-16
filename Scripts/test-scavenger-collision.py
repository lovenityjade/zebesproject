#!/usr/bin/env python3
"""Real Morph-room collision gate; controlled preceding hunt completion."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-scavenger-native.py').read_text().split('cases=0')[0]
exec(compile(source,'scavenger-collision-fixture','exec'))
apply([1,0,16]) # original Bomb location before original Morph location
assert l.sm_test_seed_room(0);wait(550)
assert l.sm_test_seed_pickup_position()
put(0xaf6,69*16+20);put(0xb10,69*16+20)
put(0xb2c,0);put(0xb2e,0)
morph=next(i for i,p in enumerate(m['placements']) if p['location']=='Morphing Ball')
before=[l.sm_seed_inventory(i) for i in range(7)]
trace=[]
for _ in range(45):
 step(64)
 trace.append(dict(frame=_,x=word(0xaf6),y=word(0xafa),handler=hex(word(0xa60)),frozen=word(0xa78),joy=hex(word(0x8b)),pose=hex(word(0xa1c)),plm=hex(word(0x1d27+68)),index=st(1)))
(root/'scavenger'/('trace-allowed.json' if st(1)>0 else 'trace-denied.json')).write_text(json.dumps(trace,indent=2))
assert not l.sm_seed_location_collected(morph) and st(1)==0
assert [l.sm_seed_inventory(i) for i in range(7)]==before
capture('collision-denied')
# Complete the preceding location in the fixture without modifying this
# room's live PLM. The actual Morph collision/grant is still entirely native.
advance=routine('sm_scavenger_pickup',C.c_int,C.c_uint)
assert advance(byhud[1]['id']) and st(1)==1
# The preceding location is in another room. VARIA's denied PLM stays in
# its draw loop until room re-entry reinstalls its collision pre-instruction.
assert l.sm_test_seed_room(0);wait(550)
assert st(1)==1
assert l.sm_test_seed_pickup_position()
put(0xaf6,69*16+20);put(0xb10,69*16+20)
put(0xb2c,0);put(0xb2e,0)
trace=[]
for _ in range(45):
 step(64)
 trace.append(dict(frame=_,x=word(0xaf6),y=word(0xafa),handler=hex(word(0xa60)),frozen=word(0xa78),joy=hex(word(0x8b)),pose=hex(word(0xa1c)),plm=hex(word(0x1d27+68)),index=st(1)))
(root/'scavenger'/('trace-allowed.json' if st(1)>0 else 'trace-denied.json')).write_text(json.dumps(trace,indent=2))
capture('collision-after-retry')
assert l.sm_seed_location_collected(morph) and st(1)==2,dict(state=l.sm_state(),progress=st(1),allowed=allows(byhud[0]['id']),x=word(0xaf6),y=word(0xafa),plms=[(i,hex(word(0x1c37+2*i)),word(0x1dc7+2*i),hex(word(0x1d27+2*i)),hex(word(0x1cd7+2*i))) for i in range(40) if word(0x1c37+2*i)])
assert [l.sm_seed_inventory(i) for i in range(7)]!=before
capture('collision-allowed')
for _ in range(600):step(1 if _%90<2 else 0)
press(8);wait(80)
assert l.sm_state()==15 and st(4)
capture('pause-prompt')
press(512);wait(3) # port button X -> native joypad $0040
assert not st(4) and st(5)==3 and st(1)==2
capture('pause-ridley')
# Native pause exit uses the original eight-frame button debounce.
for _ in range(8):step(8)
step()
for _ in range(240):
 if l.sm_state()==8:break
 step()
assert l.sm_state()==8 and not st(4) and st(1)==2,(l.sm_state(),st(4),st(1),word(0x727),word(0x763))
assert l.sm_cpu_opcodes()==0
(root/'scavenger/collision-results.json').write_text(json.dumps(dict(passed=True,
    realMorphCollision=True,deniedInventoryUnchanged=True,allowedNativeGrant=True,
    sourcePauseInputs=True,progressUnchangedByBrowsing=True,cpuOpcodes=0,
    nativeSha256=hashlib.sha256((root/'scavenger/libsm_native.so').read_bytes()).hexdigest(),scope=__doc__),indent=2)+'\n')
l.sm_shutdown();print('SCAVENGER_COLLISION_PASS',flush=True)
