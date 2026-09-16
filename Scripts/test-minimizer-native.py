#!/usr/bin/env python3
"""Three source worlds: mixed native arrivals, membership, counters, banks and rollback."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
s=(root/'test-native-objectives.py').read_text().split("for goal in audit['goals']:")[0]
s=s.replace('objectives/libsm_native.so','minimizer/libsm_native.so').replace("root/'objectives/native'","root/'minimizer/native'")
exec(compile(s,'minimizer-native-fixture','exec'))
cat=json.loads((root/'Randomizer/native_minimizer.json').read_text())
class Mini(C.Structure):
 _fields_=[('version',C.c_uint32),('size',C.c_uint32),('regions',C.c_uint16),('destinations',C.c_uint8*40),('checks',C.c_uint8*100),('reserved',C.c_uint16)]
assert C.sizeof(Mini)==152
configure_mini=l.sm_minimizer_configure;configure_mini.argtypes=[C.c_int,C.POINTER(Mini),C.c_char_p]
map_value=l.sm_map_exploration_value;map_value.argtypes=[C.c_int,C.c_int]
check=routine('sm_minimizer_check',C.c_int,C.c_int)
setup=routine('RunDoorSetupCode',None)
normal=plan([byname['nothing']]);commit(normal);pristine=C.string_at(romptr,0x300000)
plans=[];counts=[];arrivals=0
for slot in range(3):
 manifest=json.loads((root/f'minimizer/integration/seed-{slot:02d}.json').read_text())['manifest']
 mini=manifest['nativeContext']['topology']['nativeMinimizer'];obj=manifest['nativeContext']['objectives']
 p=Mini();p.version=1;p.size=C.sizeof(p);p.regions=sum(1<<r for r in mini['regions']);p.destinations[:]=mini['destinations'];p.checks[:]=mini['checks']
 assert configure_mini(slot,C.byref(p),cat['sha256'].encode())
 o=plan(obj['goals'],obj['required'],1,obj['flags']);o.version=4
 for key,field in [('itemMask','item_mask'),('beamMask','beam_mask')]:setattr(o,field,obj[key])
 for key,field in [('itemCounted','item_counted'),('areaCounted','area_counted'),('enemyTotals','enemy_totals'),('mapTotals','map_totals')]:getattr(o,field)[:]=obj[key]
 commit(o,slot,1);clear();assert l.sm_minimizer_active()
 assert [check(i) for i in range(100)]==mini['checks']
 assert map_value(-1,1)==sum(obj['mapTotals'])
 for tile in map_audit['tiles']:explore(tile)
 assert [map_value(r,0) for r in range(12)]==obj['mapTotals']
 # Counts in the native region selection match source retained checks.
 for area in range(6):assert l.sm_tracker_area_count(area,3)==sum(on and g['area']==area for on,g in zip(mini['checks'],geometry))
 for i,j in enumerate(mini['destinations']):
  desc=cat['connections'][i][j];ap=cat['accessPoints'][i];target=cat['accessPoints'][j]
  address=romptr+0x10000+ap['exit']['DoorPtr']
  expected=target['room']['RoomPtr'].to_bytes(2,'little')+bytes([desc['bitFlag'],desc['direction'],*desc['cap'],*desc['screen']])+desc['distance'].to_bytes(2,'little')+desc['asm'].to_bytes(2,'little')
  assert C.string_at(address,12)==expected,(slot,i,j)
  clear();put(0x78d,ap['exit']['DoorPtr']);put(0xaf6,111);put(0xafa,222);put(0x741,0xbeef);put(0xe1e,0x1234)
  setup();assert word(0x18a8)==128 and word(0x741)==0xbeef
  assert (word(0xaf6),word(0xafa))==((desc['x'],desc['y']) if desc['incompatible'] else (111,222))
  assert word(0xe1e)==(0 if desc['exitAsm']=='door_transition_boss_exit_fix' else 0x1234)
  arrivals+=1
 bad=Mini.from_buffer_copy(p);bad.destinations[0]=40
 assert not configure_mini(slot,C.byref(bad),cat['sha256'].encode())
 bad=Mini.from_buffer_copy(p);bad.reserved=1
 assert not configure_mini(slot,C.byref(bad),cat['sha256'].encode())
 # A pending membership edit invalidates its dependent goals at transaction time.
 modified=Mini.from_buffer_copy(p);idx=next(i for i in range(100) if o.item_counted[i]);modified.checks[idx]=0
 before=C.string_at(romptr,0x300000)
 assert configure_mini(slot,C.byref(modified),cat['sha256'].encode())
 assert not activate(slot,1) and C.string_at(romptr,0x300000)==before
 assert configure_mini(slot,C.byref(p),cat['sha256'].encode())
 plans.append((p,o));counts.append(sum(mini['checks']))
# Existing bank activation selects each committed mixed layout; no save offsets added.
for slot in [2,0,1,0]:
 assert activate(slot,1) and select(items,100,m['sha256'].encode())
 assert [check(i) for i in range(100)]==list(plans[slot][0].checks)
# Original Kraid shot callback is already in the dead state: mark immediately.
clear();put(0x79f,1);put(0xfa8,0xc537)
routine('Kraid_Shot_Mouth',None)();assert read(0xd829,1)[0]&1
# Returning through Varia's original door restores the Kraid background.
clear();put(0x78d,0x91da);put(0x741,0xbeef);setup();assert word(0x741)==0xbeef
for slot in range(3):assert configure_mini(slot,None,cat['sha256'].encode())
commit(normal);clear();assert not l.sm_minimizer_active() and C.string_at(romptr,0x300000)==pristine
put(0x79f,1);put(0xfa8,0xc537);routine('Kraid_Shot_Mouth',None)();assert not read(0xd829,1)[0]&1
assert l.sm_cpu_opcodes()==0
(root/'minimizer/native-results.json').write_text(json.dumps(dict(passed=True,arrivals=arrivals,retainedChecks=counts,filteredMapCounts=True,nativeKraidDeath=True,rollback=True,bankSwitches=4,restoration=True,cpuOpcodes=0,nativeSha256=hashlib.sha256((root/'minimizer/libsm_native.so').read_bytes()).hexdigest()),indent=2)+'\n')
l.sm_shutdown();print('MINIMIZER_NATIVE_PASS',arrivals,counts,flush=True)
