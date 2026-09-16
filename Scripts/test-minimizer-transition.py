#!/usr/bin/env python3
"""Original collision/door pipeline for one generated area-to-boss connection."""
import os,sys,textwrap
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
transition_fixture=(root/'test-minimizer-public-native.py').read_text()
exec(compile(transition_fixture.split('plans=[];counts=[];arrivals=0')[0],'minimizer-transition-fixture','exec'))
slot=0
body=transition_fixture.split('for slot in range(3):\n',1)[1].split(' for i,j in enumerate',1)[0]
exec(compile(textwrap.dedent(body),'generated-plan','exec'))
clear();assert l.sm_test_all_equipment()
i=next(i for i,j in enumerate(mini['destinations']) if i<32 and j>=32)
j=mini['destinations'][i];src=cat['accessPoints'][i];dst=cat['accessPoints'][j]
# Warp only to the departure room via its real randomized incoming descriptor.
# Destination is selected later by the original collision and door pipeline.
assert l.sm_teleport(1)
for name,value in dict(test_room_pending=src['room']['RoomPtr'],test_room_door=dst['exit']['DoorPtr'],test_room_x=src['entry']['SamusX'],test_room_y=src['entry']['SamusY']).items():
 C.c_int.from_address(base+symbols[name]).value=value
for n in range(2000):
 step()
 if l.sm_state()==8 and l.sm_room()==src['room']['RoomPtr']:break
else:raise AssertionError('Departure room did not load')
wait(440);assert l.sm_state()==8
width=word(0x7a5);height=word(0x7a7);doors=word(0x7b5);blocks=[]
for b in range(width*height):
 if word(0x10002+2*b)>>12!=9:continue
 bts=read(0x16402+b,1)[0]&127
 if int.from_bytes(C.string_at(romptr+0x70000+doors+2*bts,2),'little')==src['exit']['DoorPtr']:blocks.append(b)
assert blocks
b=blocks[len(blocks)//2];x=b%width*16+8;y=b//width*16+8
for addr,v in [(0xaf6,x),(0xafa,y),(0xb10,x),(0xb14,y),(0xdc4,b),(0xe16,0)]:put(addr,v)
collide=routine('BlockColl_Vert_Door' if src['exit']['direction']&2 else 'BlockColl_Horiz_Door',C.c_uint8,C.c_void_p)
assert collide(None)==0 and l.sm_state()==9
states=[]
for n in range(2000):
 step()
 if not states or states[-1]!=l.sm_state():states.append(l.sm_state())
 if l.sm_state()==8:break
else:raise AssertionError('Mixed transition stalled')
assert 11 in states and l.sm_room()==dst['room']['RoomPtr']
wait(180);assert l.sm_state()==8 and l.sm_room()==dst['room']['RoomPtr'] and l.sm_cpu_opcodes()==0
capture('mixed-collision-arrival')
(root/'minimizer/transition-results.json').write_text(json.dumps(dict(passed=True,seed=manifest['seed'],fingerprint=manifest['sha256'],source=src['name'],destination=dst['name'],states=states,nativeSha256=hashlib.sha256((root/'minimizer/libsm_native.so').read_bytes()).hexdigest(),scope='Controlled departure-room load and inventory; original collision and complete native area-to-boss transition; no destination injection during transition.'),indent=2)+'\n')
l.sm_shutdown();print('MINIMIZER_COLLISION_PASS',src['name'],dst['name'],flush=True)
