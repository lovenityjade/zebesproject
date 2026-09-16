#!/usr/bin/env python3
"""Reproduce seed 14092032 ceiling Morph pickup via real native block/projectile collision.
Run on gaming-pc with the preserved incident manifest in pickup-incident/seed.json.
"""
import sys,os
from pathlib import Path
root=Path(sys.argv[1]).resolve()
source=(root/'Scripts/test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
source=source.replace("l=C.CDLL(str(root/'Native/build/libsm_native.so'))","l=C.CDLL(str(root/os.environ.get('PICKUP_LIB','Native/build/libsm_native.so')))")
exec(compile(source,'tracker-fixture','exec'))
out=root/'pickup-incident'/('SMTests-ceiling-'+os.environ.get('PICKUP_LABEL','fixed'));out.mkdir(exist_ok=True)
m=json.loads((root/'pickup-incident/seed.json').read_text())['manifest'];t=m['tracker']
boot('ceiling-repro',True)
assert l.sm_test_seed_room(1);wait(550)
assert l.sm_test_seed_pickup_position()
for _ in range(45):step(64)
assert l.sm_seed_inventory(4)==0x100, 'First real pickup must grant Hi-Jump'
for i in range(700):step(256 if i%60<2 else 0)
assert not l.sm_message_active()
assert l.sm_test_room(0x9f64,456,620);wait(550)
print('ROOM',hex(l.sm_room()),'STATE',l.sm_state(),flush=True)
press(128);put(0x18a8,1000);wait(100)
capture('ceiling-before')
for i in range(300):
 step(16|512|(256 if 60<=i<120 or 180<=i<240 else 0))
 if l.sm_message_active():
  print('PICKUP',i,'message',word(0x1c1f),'INV',[l.sm_seed_inventory(k) for k in range(7)],flush=True)
  capture('ceiling-pickup');break
else:
 raise AssertionError('Ceiling pickup did not occur through shooting/jumping')
expected=int(os.environ.get('PICKUP_EXPECT_MASK','0x104'),0)
assert l.sm_seed_inventory(4)==expected and word(0x1c1f)==9
s=snap();assert s.acquired_items==expected and s.active_items==expected
checks=[geometry[i]['name'] for i,c in enumerate(s.collected) if c]
assert sorted(checks)==sorted(['Morphing Ball','Energy Tank, Brinstar Ceiling']),checks
assert not l.sm_cpu_opcodes()
report=dict(passed=True,seed=m['seed'],fingerprint=m['sha256'],room=hex(l.sm_room()),
 collected=checks,message=9,acquired=hex(s.acquired_items),equipped=hex(s.active_items),
 cpuOpcodes=0,sourceRomUnchanged=hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash,
 fixture='Native Hi-Jump pickup; positioned in actual Blue Brinstar Energy Tank room; native shooting reveals the hidden block and native jumping collects Morph; invincibility prevents enemy interference.')
(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('SM_CEILING_MORPH_PASS',json.dumps(report),flush=True)
l.sm_shutdown()
