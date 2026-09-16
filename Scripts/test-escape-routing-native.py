#!/usr/bin/env python3
"""Replay three source captures through native ordered doors and animal rooms."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
fixture=(root/'test-escape-clock-native.py').read_text().split('results=[]')[0]
exec(compile(fixture,'escape-routing-fixture','exec'))
from types import SimpleNamespace as NS
sys.path.insert(0,str(root/'Randomizer'))
from sm_escape import capture_routing,validate_routing,routing_catalog
from rom.flavor import RomFlavor
RomFlavor.factory(str(root/'Randomizer/upstream'))
escape_catalog=routing_catalog();escape_ids={a['name']:a['id'] for a in escape_catalog['accessPoints']}
class Routes(C.Structure):
 _fields_=[('version',C.c_uint32),('size',C.c_uint32),('count',C.c_uint16),('animals',C.c_uint16),('pairs',(C.c_uint8*2)*16)]
assert C.sizeof(Routes)==44
route_configure=l.sm_escape_routing_configure;route_configure.argtypes=[C.c_int,C.POINTER(Routes),C.c_char_p]
route_sha=escape_catalog['sha256'].encode()
setup_door=routine('RunDoorSetupCode',None)
setup_room=routine('RunRoomSetupCode',None)
route_results=[];saved_plans=[];case_descriptors=[]
for case in range(3):
 captured_data=json.loads((root/f'escape/capture/seed-{case:02d}.json').read_text())
 source_doors=[]
 for d in captured_data['doors']:
  actual=dict(d);actual['transition']=(NS(Name=d['source'],Escape=True),NS(Name=d['target'],Escape=True));source_doors.append(actual)
 contract=capture_routing(NS(settings=dict(escapeAttr=captured_data['attributes'],doors=source_doors),symbols=RomFlavor.symbols))
 p=Routes();p.version=1;p.size=C.sizeof(p);p.count=len(contract['pairs']);p.animals=contract['animals']
 for n,pair in enumerate(contract['pairs']):p.pairs[n][:]=pair
 assert route_configure(case,C.byref(p),route_sha);saved_plans.append(p)
 clock=captured_data['clock'];cp=Clock();cp.version=1;cp.size=C.sizeof(cp);cp.flags=clock['flags'];cp.timer=clock['timer'];cp.half_timer=clock['halfTimer'];cp.timers[:]=clock['timers'];cp.half_timers[:]=clock['halfTimers']
 assert clock_configure(case,C.byref(cp));commit(plan([byname['nothing']]),case);clear()
 # Independently retain the last actual VARIA writer entry per physical door.
 final={}
 for d in source_doors:
  door=RomFlavor.symbols.getAddress(d['DoorPtrSym'])&65535 if 'DoorPtrSym' in d else d['DoorPtr']
  final[door]=d
 for door,d in final.items():
  expected=d['RoomPtr'].to_bytes(2,'little')+bytes([d['bitFlag'],d['direction'],*d['cap'],*d['screen']])+d['distanceToSpawn'].to_bytes(2,'little')+d['doorAsmPtr'].to_bytes(2,'little')
  assert C.string_at(romptr+0x10000+door,12)==expected,(case,hex(door))
  clear();put(0x78d,door);put(0xaf6,111);put(0xafa,222);put(0x1ff34,0);mark(14)
  before=read(0,131072)
  if d['doorAsmPtr']:routine('CallDoorDefSetupCode',None,C.c_uint32)(0x8f0000|d['doorAsmPtr'])
  expected_scroll=read(0xcd20,50);C.memmove(ram,before,len(before))
  setup_door()
  assert read(0xcd20,50)==expected_scroll
  assert (word(0xaf6),word(0xafa))==((d['SamusX'],d['SamusY']) if 'SamusX' in d else (111,222))
  assert word(0x18a8)==128
  assert word(0x1ff34)==int('exitAsm' in d)
 # The real room setup reads source-authored lists; four exits wrap to zero.
 clear();mark(14)
 for cycle in range(8):
  put(0x79b,0x9879);put(0x7bb,0x98c4);setup_room()
  assert word(0x7b5)==0xf000+(cycle%4)*4
  outgoing=int.from_bytes(C.string_at(romptr+0x70000+word(0x7b5)+2,2),'little')
  assert outgoing==0xadac+24*(cycle%4)
  put(0x78d,outgoing);setup_door();assert word(0x1ff34)==(cycle+1)%4
 # No counter changes before the escape event.
 clear();put(0x78d,0xadac);put(0x1ff34,2);setup_door();assert word(0x1ff34)==2
 clear();mark(14);put(0x79b,0x9804);put(0x7bb,0x984f);put(0x7a5,16);setup_room()
 assert word(0x7b5)==0xf046 and C.string_at(romptr+0x7f046,2)==b'\x00\xae'
 assert any(word(0x1c37+i*2)==0xb9ed for i in range(40))
 clear();mark(14);put(0x79b,0xcaf6);put(0x7bb,0xcb22)
 before=read(0xcd20,50);routine('DoorCode_SetScroll_48',None)();scroll=read(0xcd20,50);C.memmove(ram+0xcd20,before,50);setup_room();assert read(0xcd20,50)==scroll
 # WS map grey door reproduces the source additional-PLM variant.
 clear();put(0x79b,0xcc6f);put(0x7bb,0xcc9b);put(0x7a5,80);setup_room()
 matches=[i for i in range(40) if word(0x1c37+i*2)==0xc848]
 assert len(matches)==1
 # Original grey-door setup extracts condition bits from its room argument.
 assert word(0x1dc7+matches[0]*2)==(0x0061 if p.animals==4 else 0x8061)
 route_results.append(dict(seed=captured_data['seed'],orderedWrites=p.count,physicalDoors=len(final),animals=p.animals,contract=contract))
 case_descriptors.append({door:C.string_at(romptr+0x10000+door,12) for door in final})
# Independent A/B/C activation and rejected updates leave the prior plan intact.
bad=Routes.from_buffer_copy(saved_plans[0]);bad.pairs[0][0]=255
assert not route_configure(0,C.byref(bad),route_sha)
assert not route_configure(0,C.byref(saved_plans[0]),b'0'*64)
for case in [0,2,1,0]:
 commit(plan([byname['nothing']]),case)
 for door,expected in case_descriptors[case].items():assert C.string_at(romptr+0x10000+door,12)==expected
for case in range(3):assert route_configure(case,None,route_sha) and clock_configure(case,None)
commit(plan([byname['nothing']]));clear()
pristine=rom.read_bytes()
for ap in escape_catalog['accessPoints']:
 a=0x10000+ap['door'];assert C.string_at(romptr+a,12)==pristine[a:a+12]
for span in json.loads((root/'escape/data-audit.json').read_text())['roomSpans']:
 a=span['address'];assert C.string_at(romptr+a,len(span['data']))==pristine[a:a+len(span['data'])]
assert l.sm_cpu_opcodes()==0
(root/'escape/routing-native-results.json').write_text(json.dumps(dict(passed=True,cases=route_results,cycles=24,independentSlots=True,restored=True,nativeSha256=hashlib.sha256((root/'escape/libsm_native.so').read_bytes()).hexdigest(),scope='Source captured door writes, native arrival/setup callbacks and animal door cycles; not a full escape playthrough.'),indent=2)+'\n')
l.sm_shutdown();print('ESCAPE_ROUTING_NATIVE_PASS',len(route_results),flush=True)
