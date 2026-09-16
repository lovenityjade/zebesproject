#!/usr/bin/env python3
"""Native Fast Tourian routing, scripted Mother Brain phases, refill and restoration."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
s=(root/'test-native-objectives.py').read_text().split("for goal in audit['goals']:")[0]
s=s.replace('objectives/libsm_native.so','fast-tourian/libsm_native.so').replace("root/'objectives/native'","root/'fast-tourian/native'")
exec(compile(s,'fast-tourian-native-fixture','exec'))
fast=l.sm_tourian_fast
setup=routine('RunDoorSetupCode',None)
start_hyper=routine('MotherBrain_Phase3_Death_10',None)
end_hyper=routine('MotherBrain_Phase3_Death_12',None)
mb2=routine('MotherBomb_FiringRainbowBeam_0',None)
refill_frame=routine('sm_tourian_frame',None)
room_setup=routine('sm_tourian_room',None)
map_value=l.sm_map_exploration_value;map_value.argtypes=[C.c_int,C.c_int]
def activate_fast(layout=0,goal='nothing'):
 p=plan([byname[goal]],layout=layout,flags=8);p.version=3
 if not layout:p.map_totals[1]-=1
 commit(p,layout=layout);clear();return p
activate_fast();frame()
l.sm_test_room.argtypes=[C.c_int,C.c_int,C.c_int]
assert l.sm_test_room(0xa5ed,1160,136);wait(550)
capture('warp-arrival')
assert word(0x79b)==0xa5ed,(hex(word(0x79b)),l.sm_state(),word(0xaf6),word(0xafa))
capture('statues-hallway')
for _ in range(300):
 step(128 | (512 if _%30<2 else 0) | (256 if _%60<16 else 0))
 if word(0x79b)==0xddc4 and l.sm_state()==8:break
capture('tourian-eye')
assert word(0x79b)==0xddc4,(hex(word(0x79b)),l.sm_state(),word(0xaf6),word(0xafa))
wait(120)
assert l.sm_state()==8 and event(161)
assert l.sm_test_all_equipment()
assert l.sm_test_room(0xdd58,700,136);wait(300)
capture('mother-brain-room')
assert word(0x79b)==0xdd58 and l.sm_state()==8,(hex(word(0x79b)),l.sm_state(),word(0x9c2),word(0xaf6),word(0xafa))
assert l.sm_cpu_opcodes()==0
(root/'fast-tourian/traversal-results.json').write_text(json.dumps(dict(passed=True,physicalStatuesToEye=True,motherBrainRoomLoads=True,cpuOpcodes=0,nativeSha256=hashlib.sha256((root/'fast-tourian/libsm_native.so').read_bytes()).hexdigest(),scope='Native controlled warp to authored hallway, physical door traversal to the shortened Tourian, and MB room load; not a full battle playthrough.'),indent=2)+'\n')
l.sm_shutdown();print('FAST_TOURIAN_TRAVERSAL_PASS',flush=True)
