#!/usr/bin/env python3
from pathlib import Path
fixture=Path(__file__).with_name('test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
exec(compile(fixture,'tracker-fixture','exec'))
out=root/'credits-stats-results';out.mkdir(exist_ok=True)
l.sm_stats_value.argtypes=[C.c_int];l.sm_stats_value.restype=C.c_uint64
boot('SMTests-stats',True)
for _ in range(20):step(64)
wait(20)
put(0x9a6,0);put(0x9d2,0)
a=l.sm_stats_value(31)
for _ in range(90):step(512)
assert l.sm_stats_value(31)>a;wait(30)
put(0x9a6,0x1000);a=l.sm_stats_value(32)
for _ in range(90):step(512)
wait(20);assert l.sm_stats_value(32)==a+1,(l.sm_stats_value(32),a)
for select,offset,id in [(1,0x9c6,34),(2,0x9ca,35)]:
 put(offset,20);put(offset+2,20);put(0x9d2,select);a=l.sm_stats_value(id)
 for i in range(100):step(512 if i%20==0 else 0)
 wait(30);used=20-word(offset);assert used>0 and l.sm_stats_value(id)==a+used,(select,used,l.sm_stats_value(id)-a)
# Native morph/bomb controls.
assert l.sm_test_combat_equipment();put(0x9d2,0)
press(32);wait(10);press(32);wait(15)
a=l.sm_stats_value(37);press(512);wait(20);assert l.sm_stats_value(37)==a+1
put(0x9d2,3);a=l.sm_stats_value(36);ammo=word(0x9ce);press(512);wait(30)
assert l.sm_stats_value(36)==a+1 and word(0x9ce)==ammo-1
wait(600)
# A real Wave special beam attack consumes one PB but has its own counter.
press(16);wait(20);put(0x9a6,0x1001);put(0x9d2,3);put(0x9ce,5)
a=l.sm_stats_value(33);pb=l.sm_stats_value(36)
for _ in range(145):step(512)
wait(30);assert l.sm_stats_value(33)==a+1 and l.sm_stats_value(36)==pb and word(0x9ce)==4
wait(180)
# Pause time belongs to the menu, not to graph-region exploration.
for _ in range(8):step(8)
for _ in range(600):
 step()
 if l.sm_state()==15:break
assert l.sm_state()==15;wait(80)
a=l.sm_stats_value(38);regions=[l.sm_stats_value(7+2*i) for i in range(12)]
wait(120);assert l.sm_stats_value(38)==a+120
assert regions==[l.sm_stats_value(7+2*i) for i in range(12)]
# A real out-of-health transition increments deaths once, without reserves.
for _ in range(8):step(8)
for _ in range(600):
 step()
 if l.sm_state()==8:break
assert l.sm_state()==8;wait(30);a=l.sm_stats_value(40)
put(0x9c0,0);put(0x9d6,0);put(0x9c2,0)
for _ in range(120):
 step()
 if l.sm_stats_value(40)>a:break
assert l.sm_stats_value(40)==a+1
assert l.sm_save();saved=[l.sm_stats_value(i) for i in range(48)]
l.sm_shutdown()
boot('SMTests-stats',True)
assert all(l.sm_stats_value(i)==saved[i] for i in [31,32,33,34,35,36,37,38,40])
assert l.sm_stats_value(41)==saved[41]+1
# Controlled natural-ending entry uses the real native setup and final Samus sequence.
assert l.sm_test_credits_ending()
for _ in range(18000):
 step()
 if l.sm_credits_state(0)==1:break
assert l.sm_credits_state(0)==1
stats=[l.sm_stats_value(i) for i in range(48)]
for _ in range(20000):
 step()
 if l.sm_credits_state(0)==0:break
assert l.sm_credits_state(0)==0
assert stats==[l.sm_stats_value(i) for i in range(48)]
wait(700);capture('post-credits-samus')
wait(900);assert not l.sm_error(),l.sm_error()
assert read(0x69,1)==b'\x11', 'Gameplay blending leaked into the ending'

assert l.sm_cpu_opcodes()==0
capture('post-credits-final')
report=dict(passed=True,actualUnchargedChargedMissilesSupersBombsPowerBombs=True,specialBeamSeparateFromPowerBombs=True,actualDeathTransition=True,pauseSeparatedFromRegions=True,persistenceAndResetCount=True,naturalCreditsComplete=True,postCreditsState=l.sm_state(),nativeCpuOpcodes=0,sourceRomUnchanged=hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash,nativeSha256=hashlib.sha256((root/'Native/build/libsm_native.so').read_bytes()).hexdigest())
l.sm_shutdown();(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n');print('CREDITS STATS PASS',flush=True)
