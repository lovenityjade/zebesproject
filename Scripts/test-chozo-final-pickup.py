#!/usr/bin/env python3
"""Physical final pickup: tablet dialog, then warning, then timed escape."""
import sys,uuid
from pathlib import Path
root=Path(sys.argv[1]).resolve()
source=(root/'test-chozo-escape-lifecycle.py').read_text().split('for minutes in (3,5,6,7,10):')[0]
exec(compile(source,'pickup-fixture','exec'))
for p in m['placements']:
 if p['location']=='Morphing Ball':p.update(item='ChozoRelic',kind=1,plm=0xef27)
boot('final-pickup-'+uuid.uuid4().hex,True)
assert l.sm_test_room(0x9e9f,69*16+40,41*16+8)
for _ in range(2000):
 step()
 if l.sm_state()==8 and l.sm_room()==0x9e9f:break
wait(440);assert l.sm_test_seed_pickup_position()
before=l.sm_relic_count();assert before==0
for _ in range(100):
 step(64)
 if l.sm_relic_count()>before:break
assert l.sm_relic_count()==1 and l.sm_message_active()
wait(80);capture('final-tablet-pickup')
assert not l.sm_relic_escape_active() and not word(0x943)
# Close only the first dialog; the warning must not start the escape clock.
for i in range(800):
 step(256 if i%60<5 else 0)
 if not l.sm_message_active():break
else:raise AssertionError('Tablet dialog did not close')
wait(80);assert l.sm_message_active() and not l.sm_relic_escape_active()
assert not word(0x943) and not (snap().events[1]&64)
assert word(0x9a2)&0x200 and word(0x9a4)&0x200
capture('final-tablet-warning')
for i in range(800):
 step(256 if i%60<5 else 0)
 if l.sm_relic_escape_active():break
else:raise AssertionError('Warning did not close')
assert read(0x947,1)[0]==5 and snap().events[1]&64
assert not l.sm_cpu_opcodes();l.sm_shutdown()
print('PHYSICAL_FINAL_TABLET_TWO_NATIVE_DIALOGS_THEN_ESCAPE_PASS',flush=True)
