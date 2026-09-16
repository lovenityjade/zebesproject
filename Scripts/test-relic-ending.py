import sys
from pathlib import Path
root=Path(sys.argv[1]);script=(root/'full-options/test-relic-runtime.py').read_text().split('# Force a visible relic')[0]
script=script.replace("root/'full-options/relic.json'","root/'full-options/verified-0.json'")
script=script.replace("exec(compile(s,'fixture','exec'))", "s=s.replace(\"shutil.copy2(root/'slot-results/bank.sram.dat',save)\",\"shutil.copy2(root/'slot-results/bank.sram.dat',save) if not save.exists() else None\")\nexec(compile(s,'fixture','exec'))")
exec(compile(script,'fixture','exec'))
import uuid
label='quota-ending-'+uuid.uuid4().hex
boot(label,True)
rom_bytes=rom.read_bytes();relics=[p for p in m['placements'] if p['kind']==1]
def grant(p):
 header=p['plm'];start=int.from_bytes(rom_bytes[0x20000+(header&32767)+2:0x20000+(header&32767)+4],'little')
 data=rom_bytes[0x20000+(start&32767):0x20000+(start&32767)+150]
 ip=start+data.index(b'\x99\x88');bit=int.from_bytes(rom_bytes[p['address']+4:p['address']+6],'little')&255
 before=l.sm_relic_count();inventory=[l.sm_seed_inventory(i) for i in range(7)]
 put(0x1c37,header);put(0x1cd7,0x8469);put(0x1d27,ip);put(0xde1c,1);put(0x1dc7,bit);put(0x1c87,0);put(0xdf0c,0)
 wait(5);assert l.sm_relic_count()==before+1
 assert l.sm_message_active()
 for i in range(600):
  step(256 if i>380 else 0)
  if not l.sm_message_active():break
 assert not l.sm_message_active()
 wait(2)
 assert inventory==[l.sm_seed_inventory(i) for i in range(7)]
 put(0x1c37,0)
for p in relics[:19]:grant(p)
assert l.sm_relic_count()==19
press(32)
for _ in range(1000):
 step()
 if l.sm_message_active():break
assert l.sm_state()==8 and l.sm_credits_state(0)==0 and l.sm_message_active(),(l.sm_state(),word(0x1c1f))
print('QUOTA_NOT_MET_NO_ENDING',flush=True)
# Default Yes on native ship save prompt. The original routine owns SaveToSram.
for i in range(1000):step(256 if i%60<2 else 0)
assert not l.sm_message_active()
assert l.sm_save();l.sm_shutdown()
boot(label,True)
assert l.sm_relic_count()==19,('reload',l.sm_relic_count())
print('RELIC_SAVE_RELOAD_PASS',flush=True)
grant(relics[19]);assert l.sm_relic_count()==20
l.sm_tracker_configure(1,1,1)
press(8)
for _ in range(1000):
 step()
 if l.sm_state()==15:break
wait(80);capture('quota-met-map')
for _ in range(8):step(8)
step()
for _ in range(1000):
 step()
 if l.sm_state()==8:break
wait(20);press(32)
seen=set()
for i in range(2500):
 step();seen.add(l.sm_state())
 if l.sm_credits_state(0)==1:break
else:raise AssertionError(('No real credits',sorted(seen),l.sm_samus_x(),l.sm_samus_y()))
assert l.sm_state()==39 and l.sm_credits_state(0)==1
assert not snap().boss_bits[5]&2
assert not snap().events[1]&64
assert not l.sm_cpu_opcodes()
wait(950);capture('quota-ending-credits')
(out/'ending-result.json').write_text(json.dumps(dict(quota=20,earlyDeparturePrevented=True,reload=19,finalCount=l.sm_relic_count(),realCredits=True,motherBrainDefeated=False,escapeEvent=False,states=sorted(seen),emulatedCpu=0),indent=2))
l.sm_shutdown();print('RELIC_REAL_ENDING_PASS',flush=True)
