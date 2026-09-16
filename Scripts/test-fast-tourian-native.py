#!/usr/bin/env python3
"""Native Fast Tourian routing, scripted Mother Brain phases, refill and restoration."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
s=(root/'test-native-objectives.py').read_text().split("for goal in audit['goals']:")[0]
s=s.replace('objectives/libsm_native.so','fast-tourian/libsm_native.so').replace("root/'objectives/native'","root/'fast-tourian/native'")
exec(compile(s,'fast-tourian-native-fixture','exec'))
fast=l.sm_tourian_fast
setup=routine('RunDoorSetupCode',None)
start_hyper=routine('MotherBrain_Phase3_Death_10',None)
end_hyper=routine('MotherBrain_Phase3_Death_12',None)
mb2=routine('MotherBomb_FiringRainbowBeam_0',None)
refill_frame=routine('sm_tourian_frame',None)
room_setup=routine('sm_tourian_room',None)
map_value=l.sm_map_exploration_value;map_value.argtypes=[C.c_int,C.c_int]
def activate_fast(layout=0,goal='nothing'):
 p=plan([byname[goal]],layout=layout,flags=8);p.version=3
 if not layout:p.map_totals[1]-=1
 commit(p,layout=layout);clear();return p
normal=plan([byname['nothing']]);commit(normal);pristine=C.string_at(romptr,0x300000)
for layout in [0,1]:
 p=activate_fast(layout);assert fast()
 assert C.string_at(romptr+0x7a616,2)==bytes.fromhex('5c aa')
 assert C.string_at(romptr+0x7ddeb,2)==bytes.fromhex('16 92')
 assert C.string_at(romptr+0x1aa5e,1)==C.string_at(romptr+0x19218,1)==b'\x40'
 assert map_value(-1,1)==sum(p.map_totals)
 # Count only the source-defined reachable tiles (G4 removed outside area).
 for tile in map_audit['tiles']:explore(tile)
 assert map_value(-1,0)==sum(p.map_totals)
 assert map_value(1,0)==p.map_totals[1]
 # Native left-eye hit instruction bypasses its hit counter in the source room.
 assert int.from_bytes(C.string_at(romptr+0x25887,2),'little')==0x8a91
 hit=routine('PlmInstr_IncrementDoorHitCounterAndJGE',C.c_void_p,C.c_void_p,C.c_uint16)
 put(0x79b,0xddc4);put(0x1dc7,0xa8);put(0x1df0c,0)
 result=hit(romptr+0x25889,0)
 assert result==romptr+0x2588c and word(0x1df0c)==0 and not read(0xd8b0+0x15,1)[0]&1
 # Source skip_check is 84:8AA3 (INY x3; RTS), so shots cannot open it.
 assert C.string_at(romptr+0x20aa3,4)==bytes.fromhex('c8 c8 c8 60')
 # Actual original door setup entry; objective reveal and conditional latch.
 clear();put(0x78d,0xaa5c);setup();assert event(161) and not read(0xd8b0+0x15,1)[0]&1
 mark(10);setup();assert read(0xd8b0+0x15,1)[0]&1
 put(0x9c2,1);put(0x9c4,999);put(0x9c6,1);put(0x9c8,100)
 put(0x78d,0xaaa4);setup();assert all(event(i) for i in range(2,6))
 assert word(0x9c2)==(999 if layout else 1) and word(0x9c6)==(100 if layout else 1)
 # Exact native MB2 script exits toward the phase-3 death path with zero HP.
 clear();mb2();assert word(0xfa8)==0xc1cf
 put(0xfb2,0);put(0x9c4,999);put(0x9c2,1);put(0x9d4,200);put(0x9d6,0)
 for a in [0xcd0,0xcd6,0xcd8,0xcda,0xcdc,0xcde,0xce0]:put(a,9)
 start_hyper();assert word(0xfa8)==0xb189 and word(0xa4a)==0x8000
 assert all(word(a)==0 for a in [0xcd0,0xcd6,0xcd8,0xcda,0xcdc,0xcde,0xce0])
 assert word(0x1ff36)==3 and word(0x9a6)==4105
 put(0x79b,0xdd58);refill_frame();assert word(0x9c2)==4
 put(0x9c2,998);refill_frame();assert word(0x9c2)==999
 put(0xfb2,0);end_hyper();assert word(0xfa8)==0xb1d5 and word(0xa4a)==word(0x1ff36)==0
 assert word(0x9c2)==999 and word(0x9d6)==200
 put(0x1ff36,9);room_setup();assert word(0x1ff36)==0
commit(normal);clear();assert not fast() and C.string_at(romptr,0x300000)==pristine
mb2();assert word(0xfa8)==0xb91a
assert l.sm_cpu_opcodes()==0
(root/'fast-tourian/native-results.json').write_text(json.dumps(dict(passed=True,layouts=2,romRestoration=True,nativeMotherBrainScripts=True,cpuOpcodes=0,nativeSha256=hashlib.sha256((root/'fast-tourian/libsm_native.so').read_bytes()).hexdigest(),scope=__doc__),indent=2)+'\n')
l.sm_shutdown();print('FAST_TOURIAN_NATIVE_PASS',flush=True)
