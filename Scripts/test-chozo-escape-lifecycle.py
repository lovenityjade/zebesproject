#!/usr/bin/env python3
"""Isolated gaming-pc regression; never use player save files."""
import sys,uuid
from pathlib import Path
root=Path(sys.argv[1]);script=(root/'full-options/test-relic-runtime.py').read_text().split('# Force a visible relic')[0]
script=script.replace("root/'full-options/libsm_native.so'","root/'Native/build/libsm_native.so'").replace("root/'full-options/relic.json'","root/'full-options/verified-0.json'")
script=script.replace('l.sm_relic_configure(1,20)','l.sm_relic_configure(1,1)')
script=script.replace("exec(compile(s,'fixture','exec'))", "s=s.replace(\"shutil.copy2(root/'slot-results/bank.sram.dat',save)\",\"shutil.copy2(root/'slot-results/bank.sram.dat',save) if not save.exists() else None\")\nexec(compile(s,'fixture','exec'))")
exec(compile(script,'fixture','exec'))
raw=rom.read_bytes();relic=next(p for p in m['placements'] if p['kind']==1)
bit=int.from_bytes(raw[relic['address']+4:relic['address']+6],'little')&255
for minutes in (3,5,6,7,10):
 label='escape-duration-'+str(minutes)+'-'+uuid.uuid4().hex
 boot(label,True)
 assert l.sm_relic_escape_configure(1,minutes)
 assert not l.sm_relic_escape_configure(1,4)
 assert not l.sm_relic_escape_active()
 off=0xd870+bit//8;C.memmove(ram+off,bytes([read(off,1)[0]|(1<<(bit&7))]),1)
 step();assert l.sm_relic_count()>=1 and not l.sm_relic_escape_active()
 assert word(0x9a4)&0x200 and word(0x9a2)&0x200
 wait(80);assert l.sm_message_active() and not l.sm_relic_escape_active()
 assert word(0x943)==0, 'Timer started before warning acknowledgement'
 assert not (snap().events[1]&64), 'Escape event fired before warning acknowledgement'
 capture('escape-warning')
 for i in range(800):
  step(256 if i%60<5 else 0)
  if l.sm_relic_escape_active():break
 else:raise AssertionError('Warning did not lead to escape')
 assert read(0x947,1)[0]==(minutes//10)*16+minutes%10
 wait(220);assert word(0x943)==0x8006
 if minutes==5:
  capture('escape-timer')
  C.memmove(ram+0x945,bytes([0x42,0x17,0x02]),3)
  assert l.sm_test_playtest(1,0)==1;l.sm_shutdown()
  boot(label,True)
  assert l.sm_relic_escape_active()
  bcd=lambda n:(n>>4)*10+(n&15)
  remain=bcd(read(0x947,1)[0])*60+bcd(read(0x946,1)[0])
  assert 125<=remain<=137,remain
  print('ESCAPE_SAVED_REMAINING_RESTORED',remain,flush=True)
 if minutes==10:
  C.memmove(ram+0x945,bytes([1,0,0]),3)
  seen=set()
  for _ in range(600):step();seen.add(l.sm_state())
  assert any(i>=19 for i in seen),seen
  print('ESCAPE_NATIVE_TIMEOUT_PASS',sorted(seen),flush=True)
 assert not l.sm_cpu_opcodes()
 l.sm_shutdown();print('ESCAPE_DURATION_PASS',minutes,flush=True)
