#!/usr/bin/env python3
"""Rehearse quota handling on a private backup; never edit the player's bank."""
import sys,uuid
from pathlib import Path
root=Path(sys.argv[1]).resolve()
backup=root/'escape-repair-backup/profile'
source=(root/'test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
source=source.replace("('plm',C.c_uint16)]","('plm',C.c_uint16),('kind',C.c_uint16)]")
source=source.replace("Item(i['address'],i['plm'])","Item(i['address'],i['plm'],i.get('kind',0))")
source=source.replace("root/'results/seed-14092026.json'","backup/'slots/01A0A93D141474E2A576F45848CE2FCB/seed.json'")
source=source.replace("out=root/'tracker-results'","out=root/'chozo-player/SMTests'").replace('out.mkdir(exist_ok=True)','out.mkdir(exist_ok=True,parents=True)')
source=source.replace("root/'slot-results/bank.sram.dat'","backup/'bank.sram.dat'")
source=source.replace('word(0x952)==1','word(0x952)==0')
source=source.replace('put(0x79f,0);put(0x78b,0);put(0xd914,5);put(0x998,6)','put(0xd914,5);put(0x998,6)')
source=source.replace('ram=l.sm_simulation_ram();l.sm_set_widescreen(1)', '''ram=l.sm_simulation_ram();l.sm_set_widescreen(1)
 l.sm_slots_enable(None,None)
 assert l.sm_slots_set(0,1,1,items,100,m['sha256'].encode())
 l.sm_relic_configure(0,5);assert l.sm_relic_escape_configure(0,5)''')
exec(compile(source,'player-copy-fixture','exec'))
original=hashlib.sha256((backup/'bank.sram.dat').read_bytes()).hexdigest()
boot('quota-'+uuid.uuid4().hex,True)
assert l.sm_relic_count()==4 and l.sm_relic_required()==5
assert not l.sm_relic_escape_active() and not (snap().events[1]&64)
assert not word(0x9a4)&0x200
before=snap();raw=rom.read_bytes()
# Model the next tablet's collection bit, keeping the exact saved inventory,
# seed placements and other native flags. Physical PLM pickup has its own test.
next_item=next(p for i,p in enumerate(m['placements']) if p.get('kind')==1 and not (read(0xd870+(raw[p['address']+4]>>3),1)[0]&(1<<(raw[p['address']+4]&7))))
bit=raw[next_item['address']+4];off=0xd870+bit//8
C.memmove(ram+off,bytes([read(off,1)[0]|1<<(bit&7)]),1)
step();wait(80)
assert l.sm_message_active() and not l.sm_relic_escape_active()
assert l.sm_relic_count()==5 and word(0x9a4)==(before.acquired_items|0x200)
assert word(0x9a2)&0x200 and not word(0x943)
after=snap()
for field in ['acquired_beams','max_health','max_missiles','max_supers','max_power_bombs','max_reserve','boss_bits']:
 assert bytes(getattr(before,field))==bytes(getattr(after,field)) if field=='boss_bits' else getattr(before,field)==getattr(after,field),field
capture('player-fifth-tablet-warning')
for i in range(800):
 step(256 if i%60<5 else 0)
 if l.sm_relic_escape_active():break
else:raise AssertionError('Escape did not start')
assert read(0x947,1)[0]==5
assert not l.sm_cpu_opcodes();l.sm_shutdown()
assert hashlib.sha256((backup/'bank.sram.dat').read_bytes()).hexdigest()==original
print('PLAYER_SEED_1008746831_QUOTA_4_TO_5_GIFT_WARNING_ESCAPE_UNCHANGED_BACKUP_PASS',flush=True)
