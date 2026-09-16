#!/usr/bin/env python3
"""Isolated gaming-pc regression; never use player save files."""
import sys,uuid
from pathlib import Path
root=Path(sys.argv[1]);s=(root/'test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
s=s.replace("out=root/'tracker-results'","out=root/'door-hud/SMTests'").replace('out.mkdir(exist_ok=True)','out.mkdir(exist_ok=True,parents=True)')
exec(compile(s,'fixture','exec'));timelines=[]
for wide in (0,1):
 boot('door-'+str(wide)+'-'+uuid.uuid4().hex,False);l.sm_set_widescreen(wide)
 assert l.sm_teleport(1)
 for _ in range(440):step()
 assert l.sm_test_all_equipment()
 wait(20);reference=None;seen=0;timeline=[]
 for t in range(750):
  if l.sm_state()==8 and l.sm_brightness()==15:reference=C.string_at(l.sm_wide_overlay(),400*32*4)
  step(128 | (512 if t%20<10 else 0) | (256 if 70<=t<100 else 0))
  timeline.append((l.sm_state(),l.sm_room(),l.sm_samus_x(),l.sm_samus_y()))
  if l.sm_state() in (9,10,11):
   seen+=1
   if wide and reference:
    hud=C.string_at(l.sm_wide_overlay(),400*32*4);brightness=l.sm_brightness()
    expected=bytes(v if i%4==3 else v*brightness//15 for i,v in enumerate(reference))
    assert hud==expected,('HUD shifted during door',t,l.sm_state(),brightness)
  if wide and t in (85,130,200):capture('door-'+str(t))
 assert seen and l.sm_state()==8 and l.sm_room()!=0x93d5
 timelines.append(timeline);l.sm_shutdown();print('NATIVE_DOOR_HUD_STABLE_PASS',wide,seen,flush=True)
assert timelines[0]==timelines[1]
print('NATIVE_DOOR_TIMING_SIMULATION_PARITY_PASS',flush=True)
