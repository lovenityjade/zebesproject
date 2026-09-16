#!/usr/bin/env python3
"""Native run through Speed Booster Hall; observe actual boost activation."""
import sys, uuid
from pathlib import Path
root=Path(sys.argv[1]).resolve()
fixture=(root/'test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
fixture=fixture.replace("out=root/'tracker-results'","out=root/'speed-effects/SMTests'")
fixture=fixture.replace('out.mkdir(exist_ok=True)','out.mkdir(exist_ok=True,parents=True)')
exec(compile(fixture,'fixture','exec'))
boot(uuid.uuid4().hex,False)
assert l.sm_test_room(0xacf0,3000,392)
wait(440)
assert l.sm_room()==0xacf0 and l.sm_state()==8
assert l.sm_test_all_equipment()
capture('before')
frames=impacts=0
for i in range(480):
    step(64|1)
    if l.sm_visual_state(47,0):
        frames+=1
        if frames==10:capture('boost')
    impacts+=sum(l.sm_visual_state(17,j)==144 for j in range(l.sm_visual_state(14,0)))
capture('after')
print('SPEED_EFFECTS_OBSERVED',frames,impacts,l.sm_room(),l.sm_samus_x(),l.sm_samus_y(),flush=True)
assert frames>20,frames
assert not l.sm_cpu_opcodes()
l.sm_shutdown()
print('SPEED_EFFECTS_NATIVE_BOOST_PASS',flush=True)
