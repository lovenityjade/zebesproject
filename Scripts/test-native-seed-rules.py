#!/usr/bin/env python3
"""Call actual compiled native routines in an isolated initialized game.
Symbol offsets are obtained from this exact unstripped library; no test-only
replacement of door or boss logic is compiled into the game.
"""
import sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
source=(root/'test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
source=source.replace("root/'Native/build/libsm_native.so'","root/'menu-options/libsm_native.so'").replace("out=root/'tracker-results'","out=root/'menu-options/native-probes'")
exec(compile(source,'fixture','exec'))
import subprocess,uuid
symbols={line.split()[-1]:int(line.split()[0],16) for line in subprocess.check_output(['nm','-an',str(root/'menu-options/libsm_native.so')],text=True).splitlines() if len(line.split())==3 and line.split()[1].lower()=='t'}
base=C.cast(l.sm_init,C.c_void_p).value-symbols['sm_init']
def routine(name):return C.CFUNCTYPE(None)(base+symbols[name])
align=routine('DoorTransitionFunction_ScrollScreenToAlignment');irq=routine('Irq_FollowDoorTransition')
known=C.CFUNCTYPE(C.c_int)(base+symbols['sm_map_fully_known'])
start=routine('MotherBomb_FiringRainbowBeam_8_StartDrainSamus');drain=routine('MotherBomb_FiringRainbowBeam_9_DrainingSamus')
results=[]
for randomized in (False,True):
 boot('native-rules-'+uuid.uuid4().hex,randomized)
 snapshot=C.string_at(ram,0x20000)
 for flags in (0,1,2,3,4,8,16,31):
  l.sm_seed_rules_configure(flags);assert l.sm_seed_rules_capabilities()==63;assert l.sm_seed_rules_active()==(flags if randomized else 0)
  l.sm_varia_ui_configure(15)
  expected_ui=(15 & ~( (2 if flags&8 else 0) | (4 if flags&16 else 0))) if randomized else 0
  assert l.sm_varia_ui_state(0)==expected_ui
  assert bool(known())==(randomized and not flags&4)
  for direction in (0,2):
   for low in (1,2,127,128,254,255):
    C.memmove(ram,snapshot,len(snapshot));put(0x791,direction)
    axis=0x911 if direction==2 else 0x915;put(axis,256+low);align()
    amount=2 if randomized and flags&1 else 1
    expected=256+low+(min(amount,256-low) if low>=128 else -min(amount,low))
    assert word(axis)==expected,(randomized,flags,direction,low,word(axis),expected)
  # A horizontal crossing is the same 64 native scroll steps in 64 or 32 IRQs.
  C.memmove(ram,snapshot,len(snapshot));put(0x791,0);put(0x925,0);put(0x931,0);put(0x927,512);put(0x929,256)
  calls=0
  while not word(0x931)&0x8000 and calls<70:irq();calls+=1
  expected=32 if randomized and flags&1 else 64
  assert calls==expected and word(0x925)==64 and word(0x911)==512 and word(0x915)==256,(flags,calls,word(0x925))
  # First IRQ finishing the crossing must not run a second scroll after done.
  C.memmove(ram,snapshot,len(snapshot));put(0x791,0);put(0x925,63);put(0x931,0);irq();assert word(0x925)==64
  for varia in (0,1):
   C.memmove(ram,snapshot,len(snapshot));C.memset(ram+0xf78,0,0x800);C.memset(ram+0x7800,0,0x1000)
   put(0x9c2,1500);put(0x9a2,varia);start();frames=1
   while word(0xfb2)!=65535 and frames<400:drain();frames+=1
   expected=20 if randomized and flags&2 else 300
   assert frames==expected and word(0x9c2)==1500-expected*(2-varia),(randomized,flags,varia,frames,word(0x9c2))
   results.append(dict(randomized=randomized,flags=flags,varia=varia,rainbowFrames=frames,damage=1500-word(0x9c2),doorIrqs=calls))
  assert l.sm_cpu_opcodes()==0
  print('NATIVE_RULES_PASS',randomized,flags,flush=True)
 C.memmove(ram,snapshot,len(snapshot));l.sm_shutdown()
assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(out/'verification.json').write_text(json.dumps(dict(cases=results,alignmentCases=192,emulatedCpu=0,romUnchanged=True),indent=2));print('SM_NATIVE_SEED_RULES_PASS',flush=True)
