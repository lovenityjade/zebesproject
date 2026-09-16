#!/usr/bin/env python3
"""Confirm real native save-station interaction on isolated gaming-pc fixtures."""
import sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
source=(root/'test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
source=source.replace("root/'Native/build/libsm_native.so'","root/'Native/build/libsm_native.so'")
source=source.replace("out=root/'tracker-results'","out=root/'playtest-refill/SMTests'")
source=source.replace("shutil.copy2(root/'slot-results/bank.sram.dat',save)","shutil.copy2(root/'slot-results/bank.sram.dat',save) if not save.exists() else None")
source=source.replace('out.mkdir(exist_ok=True)','out.mkdir(exist_ok=True,parents=True)')
exec(compile(source,'fixture','exec'))
import uuid
current=[0x9c2,0x9d6,0x9c6,0x9ca,0x9ce]
maximum=[0x9c4,0x9d4,0x9c8,0x9cc,0x9d0]
caps=[399,200,30,10,5];low=[37,13,7,2,1]
results=[]
for randomized in (False,True):
 for enabled,accept in [(False,True),(True,False),(True,True)]:
  label=f'{randomized}-{enabled}-{accept}-'+uuid.uuid4().hex
  boot(label,randomized)
  assert l.sm_refill_before_save()==0 if not results else True
  l.sm_set_refill_before_save(enabled);assert bool(l.sm_refill_before_save())==enabled
  for a,v in zip(maximum,caps):put(a,v)
  for a,v in zip(current,low):put(a,v)
  assert l.sm_teleport(1)
  for _ in range(2000):
   step()
   if l.sm_state()==8 and l.sm_room()==0x93d5:break
  wait(440)
  print('STATION',randomized,enabled,accept,l.sm_samus_x(),l.sm_samus_y(),flush=True)
  # Leave the spawn pose, then place Samus above the real pad and clear the
  # post-load pad lockout to model a fresh room entry. Native block collision
  # owns the station PLM and confirmation; no save function is called by test.
  for _ in range(25):step(64)
  # Controlled position above the real pad; collision still owns activation.
  put(0xaf6,96);put(0xafa,120);put(0xb2e,0);put(0xb2c,0);put(0x1e75,0)
  for _ in range(140):
   step()
   if l.sm_message_active():break
  capture('station-probe')
  assert l.sm_message_active(),('No station prompt',l.sm_samus_x(),l.sm_samus_y())
  wait(40)
  assert [word(a) for a in current]==low,'Refill occurred before confirmation'
  if not accept:press(128)
  events=[]
  for i in range(700):
   step(256 if i%60<5 else 0)
   events += [l.sm_visual_state(17,j) for j in range(l.sm_visual_state(14,0))]
  if accept:assert 142 in events,events
  expected=caps if enabled and accept else low
  assert [word(a) for a in current]==expected,([word(a) for a in current],expected)
  capture(f'{randomized}-{enabled}-{accept}')
  if accept:
   assert l.sm_save();l.sm_shutdown();boot(label,randomized)
   assert [word(a) for a in current]==expected,('Reload',[word(a) for a in current],expected)
   assert [word(a) for a in maximum]==caps
  assert not l.sm_cpu_opcodes()
  results.append(dict(randomized=randomized,enabled=enabled,accepted=accept,current=expected,reloaded=accept))
  l.sm_shutdown()
assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(out/'verification.json').write_text(json.dumps(dict(passed=True,cases=results,sourceRomUnchanged=True,emulatedCpu=0),indent=2))
print('SM_SAVE_REFILL_PASS',flush=True)
