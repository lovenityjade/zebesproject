#!/usr/bin/env python3
"""Actual PLM routines, controlled RAM fixtures, isolated gaming-pc only."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
(root/'tweaks').mkdir(exist_ok=True)
source=(root/'test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
source=source.replace("root/'Native/build/libsm_native.so'","root/'world-data/libsm_native.so'").replace("out=root/'tracker-results'","out=root/'tweaks/native'")
source=source.replace("('plm',C.c_uint16)]", "('plm',C.c_uint16),('kind',C.c_uint16)]").replace("Item(i['address'],i['plm'])", "Item(i['address'],i['plm'],i.get('kind',0))")
exec(compile(source,'tweak-fixture','exec'))
import subprocess,uuid,itertools
libpath=root/'world-data/libsm_native.so'
symbols={p[-1]:int(p[0],16) for line in subprocess.check_output(['nm','-an',str(libpath)],text=True).splitlines() if len(p:=line.split())==3 and all(c in '0123456789abcdefABCDEF' for c in p[0])}
base=C.cast(l.sm_init,C.c_void_p).value-symbols['sm_init']
def routine(name,result,*args):return C.CFUNCTYPE(result,*args)(base+symbols[name])
activate=routine('sm_start_activate',C.c_int,C.c_int,C.c_int)
select=routine('sm_seed_select_plan',C.c_int,C.POINTER(Item),C.c_int,C.c_char_p)
door=routine('PlmInstr_JumpIfSamusHasNoBombs',C.c_void_p,C.c_void_p,C.c_uint16)
statue=routine('PlmPreInstr_WakePlmIfSamusHasBombs',None,C.c_uint16)
hand=routine('PlmSetup_D6DA_LowerNorfairChozoHandTrigger',C.c_uint8,C.c_uint16)
delete=routine('PlmInstr_Delete',C.c_void_p,C.c_void_p,C.c_uint16)
catalog=json.loads((root/'Randomizer/native_world_data.json').read_text());ids={p['name']:p['id'] for p in catalog['patches']}
l.sm_start_configure.argtypes=[C.c_int,C.c_int,C.POINTER(C.c_uint8),C.c_int,C.c_char_p]
boot('SMTests-'+uuid.uuid4().hex,True)
items=(Item*100)(*[Item(p['address'],p['plm'],p.get('kind',0)) for p in m['placements']])
baseline=read(0,131072);results=[]
for slot,names in enumerate([[],['bomb_torizo.ips'],['LN_Chozo_SpaceJump_Check_Disable','ln_chozo_platform.ips']]):
 patches=(C.c_uint8*len(names))(*[ids[n] for n in names]);assert l.sm_start_configure(slot,0,patches,len(patches),catalog['sha256'].encode())
def state(slot,randomized=True):
 C.memmove(ram,baseline,len(baseline));assert activate(slot,1)
 assert select(items,100,m['sha256'].encode()) if randomized else select(None,0,None)

# Preparing another selector is not a committed seed/world transaction.
# A rejected item table must retain the active native behavior as well as ROM.
state(0);assert activate(1,1)
bad=(Item*100)(*items);bad[0].address=0
assert not select(bad,100,m['sha256'].encode())
put(0x9a4,0);put(0x1c83,0)
put(0x1d27,0xd300);statue(0);assert word(0x1d27)==0xd300
assert select(items,100,m['sha256'].encode())
statue(0);assert word(0x1d27)==0xd302

# Slot A has neither tweak, B only Torizo, C only Chozo. Revisit B after C,
# and also choose Vanilla while the B/C world selector is still configured.
for slot,randomized in [(0,True),(1,True),(2,True),(1,True),(1,False)]:
 for bombs,item_present in itertools.product([False,True],repeat=2):
  state(slot,randomized);put(0x9a4,0x1000 if bombs else 0);put(0x1c83,0xef83 if item_present else 0)
  put(0x1d27,0xd300);put(0xde1c,25);put(0x1cd7,0xd33b)
  target=(C.c_uint8*2)(0x34,0x92);answer=door(C.addressof(target),0)
  expected=(not item_present) if slot==1 and randomized else bombs
  assert (answer==C.addressof(target)+2)==expected,(slot,randomized,bombs,item_present,answer)
  statue(0);assert word(0x1d27)==(0xd302 if expected else 0xd300)
  assert word(0xde1c)==(1 if expected else 25) and word(0x1cd7)==(0xd356 if expected else 0xd33b)
  results.append(dict(kind='torizo',slot=slot,randomized=randomized,bombs=bombs,itemPresent=item_present,wake=expected))

# Use the native deletion instruction to transition the item header. This
# proves the trigger follows the pickup PLM lifecycle, not the Bombs bit.
state(1);put(0x9a4,0);put(0x1c83,0xef83);put(0x1d27,0xd300);put(0xde1c,25);put(0x1cd7,0xd33b)
statue(0);assert word(0x1d27)==0xd300
delete(None,76);assert word(0x1c83)==0
statue(0);assert word(0x1d27)==0xd302 and word(0x9a4)==0

# The Space Jump bypass must retain the original downward collision and
# morphed-pose requirements. Other poses/directions still do nothing.
for slot,randomized in [(0,True),(2,True),(1,True),(2,False)]:
 for spacejump,pose,direction in itertools.product([False,True],[0x1d,0x79,0x7a,1],[3,0]):
  state(slot,randomized);put(0x9a4,0x200 if spacejump else 0);put(0xa1c,pose);put(0xb02,direction)
  put(0xd820,0);put(0xfb4,0);put(0x1c87,0);put(0x10002,0xb123);put(0x1c37,0xd6da)
  expected=(spacejump or (slot==2 and randomized)) and pose!=1 and direction==3
  assert hand(0)==1
  assert bool(word(0xd820)&0x1000)==expected,(slot,randomized,spacejump,pose,direction)
  assert word(0xfb4)==(1 if expected else 0)
  assert word(0x10002)==(0x123 if expected else 0xb123) and word(0x1c37)==0
  results.append(dict(kind='chozo',slot=slot,randomized=randomized,spaceJump=spacejump,pose=pose,direction=direction,activated=expected))

assert l.sm_cpu_opcodes()==0;l.sm_shutdown();assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(out/'verification.json').write_text(json.dumps(dict(cases=results,pickupPlmLifecycle=True,uncommittedSelectionIsolated=True,cpu=0,sourceRomUnchanged=True,nativeSha256=hashlib.sha256(libpath.read_bytes()).hexdigest(),scope='Real C PLM routines after native boot, controlled RAM conditions; not a manual room traversal'),indent=2))
print('NATIVE_TWEAKS_PASS',len(results),flush=True)
