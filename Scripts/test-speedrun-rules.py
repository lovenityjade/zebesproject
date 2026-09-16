#!/usr/bin/env python3
"""Isolated gaming-pc regression; never use player save files."""
import sys,uuid
from pathlib import Path
root=Path(sys.argv[1]);s=(root/'test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
s=s.replace("out=root/'tracker-results'","out=root/'speedrun-refill/SMTests'").replace('out.mkdir(exist_ok=True)','out.mkdir(exist_ok=True,parents=True)')
s=s.replace('ram=l.sm_simulation_ram();l.sm_set_widescreen(1)','ram=l.sm_simulation_ram();l.sm_set_widescreen(1);assert l.sm_run_configure(1,category)')
exec(compile(s,'fixture','exec'))
for category in (1,2):
 boot('category-'+str(category)+'-'+uuid.uuid4().hex,False)
 assert l.sm_run_state(0)==category
 l.sm_set_refill_before_save(1)
 l.sm_set_assisted_walljump(1);l.sm_set_assisted_spacejump(1)
 assert bool(l.sm_assisted_walljump())==(category==2)
 assert bool(l.sm_assisted_spacejump())==(category==2)
 for a,v in zip([0x9c4,0x9d4,0x9c8,0x9cc,0x9d0],[399,200,30,10,5]):put(a,v)
 current=[0x9c2,0x9d6,0x9c6,0x9ca,0x9ce];low=[37,13,7,2,1]
 for a,v in zip(current,low):put(a,v)
 assert l.sm_teleport(1)
 for _ in range(2000):
  step()
  if l.sm_state()==8 and l.sm_room()==0x93d5:break
 wait(440)
 for _ in range(25):step(64)
 put(0xaf6,96);put(0xafa,120);put(0xb2e,0);put(0xb2c,0);put(0x1e75,0)
 for _ in range(140):
  step()
  if l.sm_message_active():break
 assert l.sm_message_active()
 wait(40)
 for i in range(700):step(256 if i%60<5 else 0)
 assert [word(a) for a in current]==low
 assert not l.sm_run_state(2),'Debug teleport did not invalidate run'
 l.sm_shutdown();print('SPEEDRUN_REFILL_BLOCKED_ASSIST_RULES_PASS',category,flush=True)
