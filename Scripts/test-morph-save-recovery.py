#!/usr/bin/env python3
"""Load the incident recovery copy and verify its two checks, native inventory and live routes on gaming-pc."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
src=(root/'Scripts/test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
src=src.replace("l=C.CDLL(str(root/'Native/build/libsm_native.so'))","l=C.CDLL(str(root/'pickup-incident/libsm_native-fixed.so'))")
src=src.replace("root/'slot-results/bank.sram.dat'","root/'pickup-incident/repaired-bank.sram.dat'")
src=src.replace('word(0x952)==1','word(0x952)==0')
exec(compile(src,'tracker-fixture','exec'))
out=root/'pickup-incident/SMTests-recovery';out.mkdir(exist_ok=True)
m=json.loads((root/'pickup-incident/seed.json').read_text())['manifest'];t=m['tracker']
boot('recovery',True)
s=snap();assert s.slot==0 and s.acquired_items==0x104,(s.slot,hex(s.acquired_items))
assert s.active_items==0x104
checks=[geometry[i]['name'] for i,c in enumerate(s.collected) if c]
assert sorted(checks)==sorted(['Morphing Ball','Energy Tank, Brinstar Ceiling']),checks
l.sm_tracker_configure(1,1,1);states=query(s)
reachable=[geometry[i]['name'] for i,state in enumerate(states) if state==1 and not s.collected[i]]
assert reachable,reachable
# Regression oracle: erroneous Spring Ball bit loses routes in this same seed.
put(0x9a2,0x102);put(0x9a4,0x102);wrong=query(snap())
blocked_before=[geometry[i]['name'] for i,state in enumerate(states) if state==1 and wrong[i]!=1 and not s.collected[i]]
assert blocked_before,blocked_before
put(0x9a2,0x104);put(0x9a4,0x104);query(snap());press(8);wait(100)
assert l.sm_state()==15;capture('recovered-map-and-tracker')

for _ in range(8):step(2048)
wait(110);assert l.sm_pause_data(12)==1;capture('recovered-equipment')
assert not l.sm_cpu_opcodes()
report=dict(passed=True,seed=m['seed'],fingerprint=m['sha256'],slot=0,acquired='0104',equipped='0104',checks=checks,reachable=reachable,unblocked=blocked_before,cpuOpcodes=0)
(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('SM_PICKUP_RECOVERY_PASS',json.dumps(report),flush=True);l.sm_shutdown()
