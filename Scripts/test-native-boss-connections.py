#!/usr/bin/env python3
"""All 24 boss permutations and native setup routines, isolated gaming-pc."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-tweaks.py').read_text().split('for slot,names in enumerate')[0]
source=source.replace("(root/'tweaks').mkdir(exist_ok=True)","(root/'connections').mkdir(exist_ok=True)").replace("root/'tweaks/native'","root/'connections/native'").replace('world-data/libsm_native.so','connections/libsm_native.so')
exec(compile(source,'connection-fixture','exec'))
import itertools
catalog=json.loads((root/'Randomizer/native_connections.json').read_text());aps=catalog['accessPoints']
inside=[a['id'] for a in aps if a['inside']];outside=[a['id'] for a in aps if not a['inside']]
l.sm_connections_configure.argtypes=[C.c_int,C.POINTER(C.c_uint8),C.c_int,C.c_char_p]
activate=routine('sm_connections_activate',None,C.c_int)
setup=routine('RunDoorSetupCode',None)
spark=routine('Samus_EndSuperJump',C.c_uint8)
romptr=C.c_void_p.from_address(base+symbols['g_rom']).value
assert select(None,0,None);original_runtime=C.string_at(romptr,0x300000)
def configure(slot,mapping):
 values=(C.c_uint8*len(mapping))(*mapping)
 return l.sm_connections_configure(slot,values,len(values),catalog['sha256'].encode())
cases=[]
for permutation in itertools.permutations(inside):
 mapping=[0]*8
 for a,b in zip(outside,permutation):mapping[a]=b;mapping[b]=a
 assert configure(0,mapping);activate(0)
 expected=bytearray(original_runtime)
 for i,j in enumerate(mapping):
  c=catalog['connections'][i][j];room=aps[j]['room']['RoomPtr'];address=0x10000+aps[i]['exit']['DoorPtr']
  entry=room.to_bytes(2,'little')+bytes([c['bitFlag'],c['direction'],*c['cap'],*c['screen']])+c['distance'].to_bytes(2,'little')+c['asm'].to_bytes(2,'little')
  expected[address:address+12]=entry
  if aps[i]['exit']['DoorPtr']==0x91ce:expected[0x70008+room]=2
  for a in aps[j]['room'].get('songs',[]):expected[0x70000+a:0x70002+a]=bytes([aps[j]['entry']['song'],5])
 for p in m['placements']:expected[p['address']:p['address']+2]=p['plm'].to_bytes(2,'little')
 assert select(items,100,m['sha256'].encode())
 assert C.string_at(romptr,0x300000)==expected
 # Invalid candidate tables/configurations cannot replace the committed world.
 wrong=(Item*100)(*items);wrong[0].address=0
 assert not select(wrong,100,m['sha256'].encode()) and C.string_at(romptr,0x300000)==expected
 assert not configure(0,[0]*8) and not configure(0,[1,0])
 assert not l.sm_connections_configure(0,(C.c_uint8*8)(*mapping),8,b'0'*64)
 transitions=[]
 for i,j in enumerate(mapping):
  C.memmove(ram,baseline,len(baseline));c=catalog['connections'][i][j]
  put(0x78d,aps[i]['exit']['DoorPtr']);put(0xaf6,111);put(0xafa,222);put(0x9c2,999)
  for a in [0xb2c,0xb2e,0xb42,0xb44,0xb46,0xb48,0xa96]:put(a,7)
  put(0xa6e,2);put(0xdd0,0);put(0x741,0xbeef);put(0xe1e,0x1234);put(0x330,0);put(0x604,0xabcd)
  setup();assert word(0x18a8)==128 and word(0x741)==0xbeef
  assert (word(0xaf6),word(0xafa))==((c['x'],c['y']) if c['incompatible'] else (111,222))
  if c['incompatible']:
   assert all(word(a)==0 for a in [0xb2c,0xb2e,0xb42,0xb44,0xb46,0xb48,0xa96,0xa6e])
   assert read(0xa1c,12)==bytes(12) and read(0xa2a,6)==b'\xff'*6
   assert read(0xb10,8)==read(0xaf6,8)
  assert spark()==int(c['incompatible']) and word(0x9c2)==999
  assert spark()==0 and word(0x741)==0xbeef
  if c['asm']==0xe1fe:assert read(0xcd23,2)==bytes([0,1])
  if c['asm']==0xe3d9:assert read(0xcd20,1)==bytes([2]) and read(0xcd22,1)==bytes([2])
  if c['exitAsm']=='door_transition_kraid_exit_fix':
   assert word(0x330)==28 and word(0x604)==0xabcd
  if c['exitAsm']=='door_transition_boss_exit_fix':assert word(0xe1e)==0
  transitions.append(dict(source=aps[i]['name'],destination=aps[j]['name'],incompatible=c['incompatible']))
 assert configure(1,[]);activate(1)
 assert C.string_at(romptr,0x300000)==expected, 'Pending slot changed applied ROM'
 assert select(None,0,None) and C.string_at(romptr,0x300000)==original_runtime
 cases.append(dict(mapping=mapping,transitions=transitions))
 print('BOSS_PERMUTATION_PASS',len(cases),flush=True)
# State-specific Wrecked Ship save-door insertion. Preserve the room's RAM
# descriptor scratch space, avoid duplicates, and keep vanilla untouched.
room_hook=routine('sm_connections_room',None)
clear_plms=routine('ClearPLMs',None)
assert configure(0,mapping);activate(0);assert select(items,100,m['sha256'].encode())
plm_cases=[]
for enabled,room,state,expected_count in [(True,0xcaf6,0xcb08,1),(True,0xcaf6,0xcb22,0),(True,0xcaae,0xcb08,0),(False,0xcaf6,0xcb08,0)]:
 C.memmove(ram,baseline,len(baseline));clear_plms()
 assert select(items,100,m['sha256'].encode()) if enabled else select(None,0,None)
 put(0x79b,room);put(0x7bb,state);put(0x7a5,80)
 scratch=read(0x12,6);room_hook();room_hook();assert read(0x12,6)==scratch
 count=sum(word(0x1c37+i*2)==0xc842 for i in range(40))
 assert count==expected_count,(enabled,hex(room),hex(state),count)
 plm_cases.append(dict(randomized=enabled,room=room,state=state,count=count))
assert l.sm_cpu_opcodes()==0;l.sm_shutdown();assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(out/'verification.json').write_text(json.dumps(dict(permutations=cases,saveDoorCases=plm_cases,cpu=0,sourceRomUnchanged=True,nativeSha256=hashlib.sha256(libpath.read_bytes()).hexdigest(),scope='Controlled descriptor/setup/ROM transactions, not generated boss seeds or full room traversal'),indent=2))
