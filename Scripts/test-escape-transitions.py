#!/usr/bin/env python3
"""Actual door collision and ship ending for existing public escape seeds."""
import os,sys,subprocess,textwrap
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
if len(sys.argv)==2:
 for case in range(3):subprocess.run([sys.executable,__file__,str(root),str(case)],check=True)
 raise SystemExit(0)
slot=int(sys.argv[2]);transition_source=(root/'test-escape-objectives-native.py').read_text()
exec(compile(transition_source.split('for slot in range(3):')[0],'escape-transition-fixture','exec'))
body=transition_source.split('for slot in range(3):\n',1)[1].split(' assert map_value(-1,1)',1)[0]
body=body.replace('escape/integration/seed-','escape/public/seed-')
exec(compile(textwrap.dedent(body),'escape-generated-plan','exec'))
items=(Item*100)(*[Item(p['address'],p['plm'],p.get('kind',0)) for p in manifest['placements']])
assert select(items,100,manifest['sha256'].encode());clear();assert l.sm_test_all_equipment()
if slot:
 for ev in [72,88,96,80]:mark(ev)
 for _ in range(20):frame()
 assert event(14) and event(160)
else:
 mark(14);routine('sm_escape_setup',None)()

def warp(room,incoming,x,y):
 assert l.sm_teleport(1)
 for name,value in dict(test_room_pending=room,test_room_door=incoming,test_room_x=x,test_room_y=y).items():C.c_int.from_address(base+symbols[name]).value=value
 for _ in range(2000):
  step()
  if l.sm_state()==8 and l.sm_room()==room:break
 else:raise AssertionError(('Departure did not load',hex(room),hex(l.sm_room()),l.sm_state()))
 wait(180)

src=0 if slot==0 else escape_ids['Flyway Right 0']
dst=next(j for i,j in reversed(esc['routing']['pairs']) if i==src)
source_ap=escape_catalog['accessPoints'][src];target_ap=escape_catalog['accessPoints'][dst]
# Original internal incoming doors remain unchanged by escape routing.
warp(source_ap['room'],0xab1c if slot==0 else 0x8982,128,800 if slot==0 else 120)
assert l.sm_state()==8 and event(14)
width=word(0x7a5);height=word(0x7a7);door_list=word(0x7b5);blocks=[]
for b in range(width*height):
 if word(0x10002+2*b)>>12!=9:continue
 bts=read(0x16402+b,1)[0]&127
 if int.from_bytes(C.string_at(romptr+0x70000+door_list+2*bts,2),'little')==source_ap['door']:blocks.append(b)
assert blocks,(slot,hex(door_list),hex(source_ap['door']))
b=blocks[len(blocks)//2];x=b%width*16+8;y=b//width*16+8
for a,v in [(0xaf6,x),(0xafa,y),(0xb10,x),(0xb14,y),(0xdc4,b),(0xe16,0)]:put(a,v)
collide=routine('BlockColl_Horiz_Door',C.c_uint8,C.c_void_p)
assert collide(None)==0 and l.sm_state()==9
states=[]
for _ in range(2000):
 step()
 if not states or states[-1]!=l.sm_state():states.append(l.sm_state())
 if l.sm_state()==8:break
else:raise AssertionError(('Door transition stalled',states))
assert 11 in states and l.sm_room()==target_ap['room'],(states,hex(l.sm_room()),target_ap)
assert word(0x1ff34)==int(slot!=0)
wait(120);capture(f'escape-arrival-{slot}')
ending=None
if slot==1:
 # Position only at departure: the original down input, ship AI, launch and
 # cinematic state machine own the entire ending after this fixture warp.
 # Use the actual ship load station, so Samus exits the ship's native load
 # animation. A save-station warp into this room would wait for a nonexistent
 # save-platform PLM and leave the fixture's movement handler frozen.
 assert l.sm_teleport(0)
 for _ in range(2000):
  step()
  if l.sm_state()==8 and l.sm_room()==0x91f8:break
 else:raise AssertionError('Ship load station did not load')
 wait(500)
 assert event(14);press(32);seen=set()
 for n in range(14000):
  step();seen.add(l.sm_state())
  if l.sm_credits_state(0)==1:break
 else:raise AssertionError(('No ship ending',sorted(seen),hex(word(0xfa8)),l.sm_samus_x(),l.sm_samus_y(),word(0x943)))
 assert l.sm_state()==39 and 38 in seen
 ending=dict(realCredits=True,frames=n,states=sorted(seen),escapeEvent=bool(event(14)),controlledQuota=True)
 capture('disabled-tourian-credits')
assert l.sm_cpu_opcodes()==0
(root/f'escape/transition-{slot}.json').write_text(json.dumps(dict(passed=True,seed=manifest['seed'],fingerprint=manifest['sha256'],source=source_ap['name'],destination=target_ap['name'],states=states,ending=ending,cpuOpcodes=0,nativeSha256=hashlib.sha256((root/'escape/libsm_native.so').read_bytes()).hexdigest(),scope=__doc__),indent=2)+'\n')
l.sm_shutdown();print('ESCAPE_COLLISION_PASS',slot,ending,flush=True)
