#!/usr/bin/env python3
"""Native relic dialog, live collision, quotas, dismissal and ordinary Reserve regression.
Run on gaming-pc in the marked, isolated seed-test root only.
"""
import sys
from pathlib import Path
root=Path(sys.argv[1])
source=(root/'full-options/test-relic-runtime.py').read_text().split('# Force a visible relic')[0]
source=source.replace("root/'full-options/libsm_native.so'","root/'tablet-message/libsm_native.so'")
exec(compile(source,'fixture','exec'))
out=root/'tablet-message/SMTests';out.mkdir(parents=True,exist_ok=True)
for p in m['placements']:
 if p['location']=='Morphing Ball':p.update(item='ChozoRelic',kind=1,plm=0xef27)
rom_bytes=rom.read_bytes()
def line(row):
 words=[word(0x3000+2*(288+row*32+x)) for x in range(32)]
 chars=[]
 for w in words:
  n=w&1023
  chars.append(chr(ord('A')+n-0xe0) if 0xe0<=n<=0xf9 else str((n+1)%10) if n<10 else ' ')
 return ''.join(chars).strip()
def dismiss():
 events=[]
 for i in range(600):
  step(256 if i>380 else 0)
  events += [l.sm_visual_state(17,j) for j in range(l.sm_visual_state(14,0))]
  if not l.sm_message_active():break
 assert not l.sm_message_active(), 'Dialog did not close'
 wait(20)
 return events
boot('dialog',True);l.sm_relic_configure(1,15)
assert l.sm_test_room(0x9e9f,69*16+40,41*16+8)
for _ in range(2000):
 step()
 if l.sm_state()==8 and l.sm_room()==0x9e9f:break
wait(440);assert l.sm_test_seed_pickup_position()
before=[l.sm_seed_inventory(i) for i in range(7)]
for _ in range(100):
 step(64)
 if l.sm_message_active():break
assert l.sm_message_active() and l.sm_relic_count()==1
wait(40);assert line(0)=='CHOZO TABLET' and line(2)=='1 COLLECTED OUT OF 15',(line(0),line(2))
assert [l.sm_seed_inventory(i) for i in range(7)]==before
capture('one-of-fifteen')
assert 140 in dismiss(), 'Pickup effect lost during the native message freeze'
# Native grant entry for ordinary Reserve and the second relic. Traversal is
# injected here, but the actual PLM owns grants, bits, messages and timing.
def grant(p):
 header=p['plm'];start=int.from_bytes(rom_bytes[0x20000+(header&32767)+2:0x20000+(header&32767)+4],'little')
 data=rom_bytes[0x20000+(start&32767):0x20000+(start&32767)+150]
 ip=start+data.index(b'\x99\x88');bit=int.from_bytes(rom_bytes[p['address']+4:p['address']+6],'little')&255
 put(0x1c37,header);put(0x1cd7,0x8469);put(0x1d27,ip);put(0xde1c,1);put(0x1dc7,bit);put(0x1c87,0);put(0xdf0c,0)
 for _ in range(10):
  step()
  if l.sm_message_active():break
 assert l.sm_message_active()
 wait(40)
reserve=next(p for p in m['placements'] if p['item']=='Reserve')
grant(reserve);assert line(0)=='RESERVE TANK',line(0)
assert l.sm_seed_inventory(6)==before[6]+100
capture('ordinary-reserve');dismiss();put(0x1c37,0)
relics=[p for p in m['placements'] if p.get('kind')==1 and p['location']!='Morphing Ball']
# Populate 13 previously collected bits as a saved-progress fixture. The 15th
# tablet is still acquired by its native PLM; snapshot must include that grant.
for p in relics[:13]:
 bit=rom_bytes[p['address']+4];v=C.c_uint8.from_address(ram+0xd870+bit//8);v.value|=1<<(bit%8)
assert l.sm_relic_count()==14
grant(relics[13]);assert line(2)=='15 COLLECTED OUT OF 15',line(2)
capture('fifteen-of-fifteen');dismiss();put(0x1c37,0)
l.sm_relic_configure(1,20)
grant(relics[14]);assert line(2)=='16 COLLECTED OUT OF 20',line(2)
capture('sixteen-of-twenty');dismiss();put(0x1c37,0)
assert not l.sm_cpu_opcodes()
(out/'verification.json').write_text(json.dumps(dict(passed=True,nativeDialog=True,collisionFirstPickup=True,
 captions=['1 COLLECTED OUT OF 15','15 COLLECTED OUT OF 15','16 COLLECTED OUT OF 20'],
 reserveMessageUnchanged=True,reserveGrant=100,postDialogSpark=True,cpuOpcodes=0,
 sourceRomUnchanged=hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash),indent=2))
l.sm_shutdown();print('SM_RELIC_MESSAGE_PASS',flush=True)
