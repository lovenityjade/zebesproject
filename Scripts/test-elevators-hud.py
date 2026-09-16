#!/usr/bin/env python3
"""Isolated gaming-pc: actual elevator routines and seed-specific HUD counts."""
import sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
source=(root/'test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
source=source.replace("root/'Native/build/libsm_native.so'","root/'menu-options/libsm_native.so'").replace("out=root/'tracker-results'","out=root/'elevators-hud/native'")
source=source.replace("('plm',C.c_uint16)]", "('plm',C.c_uint16),('kind',C.c_uint16)]").replace("Item(i['address'],i['plm'])", "Item(i['address'],i['plm'],i.get('kind',0))")
exec(compile(source,'fixture','exec'))
import subprocess,uuid,ast
symbols={line.split()[-1]:int(line.split()[0],16) for line in subprocess.check_output(['nm','-an',str(root/'menu-options/libsm_native.so')],text=True).splitlines() if len(line.split())==3 and line.split()[1].lower()=='t'}
base=C.cast(l.sm_init,C.c_void_p).value-symbols['sm_init']
def routine(name):return C.CFUNCTYPE(None)(base+symbols[name])
depart=routine('Elevator_Func_2');arrive=routine('Elevator_Func3b');hud=routine('sm_varia_ui_frame')
l.sm_varia_ui_counted_configure.argtypes=[C.POINTER(C.c_uint8),C.c_int]
results=[]
for randomized in (False,True):
 boot('elevator-'+uuid.uuid4().hex,randomized)
 snapshot=C.string_at(ram,0x20000)
 for flags in (0,32,63):
  fast=randomized and bool(flags&32)
  l.sm_seed_rules_configure(flags);assert l.sm_seed_rules_capabilities()==63
  for direction in (0,2):
   C.memmove(ram,snapshot,len(snapshot));put(0xe54,0);put(0xfb4,direction);put(0xf7e,1000);put(0xf80,0x4000);put(0xf7a,300)
   put(0x998,8);put(0x741,13)
   for _ in range(5):depart()
   expected=(1000<<16)+0x4000+(1 if direction==0 else -1)*5*(0x30000 if fast else 0x18000)
   assert (word(0xf7e)<<16|word(0xf80))==expected,(randomized,flags,direction,'gameplay')
   assert word(0xafa)==word(0xf7e)-26 and word(0xafc)==0
   assert word(0x799)==(0x8000 if direction else 0)
   if fast and not direction:assert word(0x741)==0
   put(0x998,9);initial=(word(0xf7e)<<16)|word(0xf80)
   for _ in range(40):depart()
   moves=24 if fast and not direction else 40
   expected=initial+(1 if direction==0 else -1)*moves*(0x30000 if fast else 0x18000)
   assert (word(0xf7e)<<16|word(0xf80))==expected,(randomized,flags,direction,'transition',word(0xf7e),expected>>16)
   if fast and not direction:
    assert word(0x741)==24
    put(0x998,8);depart();assert word(0x741)==0 and word(0xf7e)==(expected>>16)+3
   # Both arriving directions clamp exactly to their station, even on odd distances.
   for gap in (1,2,5,17):
    C.memmove(ram,snapshot,len(snapshot));put(0xe54,0);put(0xfb4,direction);put(0xfa8,1000)
    put(0xf7e,1000-gap if direction else 1000+gap);put(0xf80,0);put(0xe18,3);put(0xe16,1)
    for frames in range(1,30):
     arrive()
     if word(0xe18)==0:break
    # Downward return uses '<', upward return uses '>=' in the original routine.
    expected_frames=int(gap/(3 if fast else 1.5))+1 if not direction else int(__import__('math').ceil(gap/(3 if fast else 1.5)))
    assert frames==expected_frames and word(0xf7e)==1000 and word(0xe16)==0,(fast,direction,gap,frames,expected_frames)
   results.append(dict(randomized=randomized,flags=flags,direction=direction,transitionMoves=moves))
 C.memmove(ram,snapshot,len(snapshot));l.sm_shutdown()
# One actual native boot per plan: counts follow original VARIA graph regions,
# exclude collected locations and change immediately with an A/B/C-style plan switch.
regions=['Ceres','Crateria','GreenPinkBrinstar','RedBrinstar','WreckedShip','Kraid','Norfair','Crocomire','LowerNorfair','WestMaridia','EastMaridia','Tourian']
rooms_ast=ast.parse((root/'Randomizer/upstream/tools/rooms.py').read_text())
rooms=next(ast.literal_eval(n.value) for n in rooms_ast.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='rooms' for t in n.targets))
room_region={r['Address']&65535:regions.index(r['GraphArea']) for r in rooms}
counts_results=[]
for fixture in sorted((root/'elevators-hud').glob('seed-*.json')):
 m=json.loads(fixture.read_text())['manifest'];boot('hud-'+uuid.uuid4().hex,True)
 l.sm_seed_rules_configure(0);l.sm_varia_ui_configure(15)
 counts=(C.c_uint8*100)(*[p['hudCounted'] for p in sorted(m['placements'],key=lambda p:p['address'])]);l.sm_varia_ui_counted_configure(counts,100)
 for region_id in range(12):
  location_indices=[i for i,c in enumerate(geometry) if room_region[c['roomPointer']]==region_id]
  if not location_indices:continue
  put(0x79b,geometry[location_indices[0]]['roomPointer']);put(0x998,8);C.memset(ram+0xd870,0,64)
  hud();expected=sum(counts[i] for i in location_indices);assert l.sm_varia_ui_state(2)==expected,(fixture.name,region_id,'initial',l.sm_varia_ui_state(2),expected)
  for i in location_indices[::2]:
   address=geometry[i]['address'];bit=int.from_bytes(rom.read_bytes()[address+4:address+6],'little')&255
   off=0xd870+bit//8;C.memmove(ram+off,bytes([read(off,1)[0]|1<<(bit%8)]),1)
  hud();expected=sum(counts[i] for i in location_indices[1::2]);assert l.sm_varia_ui_state(2)==expected
  l.sm_varia_ui_counted_configure(None,0);hud();assert l.sm_varia_ui_state(2)==len(location_indices[1::2])
  l.sm_varia_ui_counted_configure(counts,100);hud();assert l.sm_varia_ui_state(2)==expected
 counts_results.append(dict(seed=m['seed'],split=m['nativeContext']['split'],counted=sum(counts)))
 assert l.sm_cpu_opcodes()==0;l.sm_shutdown()
assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(out/'verification.json').write_text(json.dumps(dict(elevators=results,hud=counts_results,romUnchanged=True,emulatedCpu=0),indent=2));print('ELEVATORS_HUD_NATIVE_PASS',flush=True)
