#!/usr/bin/env python3
"""Generated schema-5 goals, exact map counts and authored level data, gaming-pc."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
fixture=(root/'test-escape-routing-native.py').read_text().split('for case in range(3):')[0]
exec(compile(fixture,'escape-objective-fixture','exec'))
class Mini(C.Structure):
 _fields_=[('version',C.c_uint32),('size',C.c_uint32),('regions',C.c_uint16),('destinations',C.c_uint8*40),('checks',C.c_uint8*100),('reserved',C.c_uint16)]
mini_configure=l.sm_minimizer_configure;mini_configure.argtypes=[C.c_int,C.POINTER(Mini),C.c_char_p]
mini_sha=json.loads((root/'Randomizer/native_minimizer.json').read_text())['sha256'].encode()
map_value=l.sm_map_exploration_value;map_value.argtypes=[C.c_int,C.c_int]
normal=plan([byname['nothing']]);commit(normal);pristine=C.string_at(romptr,0x300000)
data_audit=json.loads((root/'escape/data-audit.json').read_text());results=[];plans=[]
for slot in range(3):
 manifest=json.loads((root/f'escape/integration/seed-{slot:02d}.json').read_text())['manifest']
 obj=manifest['nativeContext']['objectives'];esc=obj['escape'];mini=obj.get('minimizer')
 if mini:
  mp=Mini();mp.version=1;mp.size=C.sizeof(mp);mp.regions=sum(1<<r for r in mini['regions']);mp.destinations[:]=mini['destinations'];mp.checks[:]=mini['checks']
  assert mini_configure(slot,C.byref(mp),mini_sha)
 cp=Clock();cp.version=1;cp.size=C.sizeof(cp);cp.flags=esc['clock']['flags'];cp.timer=esc['clock']['timer'];cp.half_timer=esc['clock']['halfTimer'];cp.timers[:]=esc['clock']['timers'];cp.half_timers[:]=esc['clock']['halfTimers']
 rp=Routes();rp.version=1;rp.size=C.sizeof(rp);rp.count=len(esc['routing']['pairs']);rp.animals=esc['routing']['animals']
 for i,pair in enumerate(esc['routing']['pairs']):rp.pairs[i][:]=pair
 assert clock_configure(slot,C.byref(cp)) and route_configure(slot,C.byref(rp),route_sha)
 op=plan(obj['goals'],obj['required'],int(obj['areaLayout']),obj['flags']);op.version=5
 for a,b in [('itemMask','item_mask'),('beamMask','beam_mask')]:setattr(op,b,obj[a])
 for a,b in [('itemCounted','item_counted'),('areaCounted','area_counted'),('enemyTotals','enemy_totals'),('mapTotals','map_totals')]:getattr(op,b)[:]=obj[a]
 commit(op,slot,int(obj['areaLayout']));clear()
 assert map_value(-1,1)==sum(obj['mapTotals'])
 for tile in map_audit['tiles']:explore(tile)
 assert [map_value(r,0) for r in range(12)]==obj['mapTotals']
 assert [map_value(r,1) for r in range(12)]==obj['mapTotals']
 # Original native decompressor consumes the audited source WS stream.
 buf=C.create_string_buffer(36866);routine('DecompressToMem',None,C.c_uint32,C.c_void_p)(0xc4bdc0,C.addressof(buf))
 for c in data_audit['wsLevel']['changes']:assert buf.raw[c['offset']]==c['after']
 clear();before=read(0x9a2,10)
 for ev in [72,88,96,80]:mark(ev)
 for _ in range(20):frame()
 assert state(4) and bool(event(14))==(slot!=0)
 if slot:
  assert event(160) and word(0x943)==2 and read(0x9a2,10)==before
  assert read(0xd8b0,32)==b'\xff'*32
 # Native dependency revalidation rejects a cleared routing plan atomically.
 before=C.string_at(romptr,0x300000);assert route_configure(slot,None,route_sha)
 assert not activate(slot,1) and C.string_at(romptr,0x300000)==before
 assert route_configure(slot,C.byref(rp),route_sha)
 plans.append((op,bool(mini)));results.append(dict(seed=manifest['seed'],fingerprint=manifest['sha256'],mapTotals=obj['mapTotals'],completionMode=manifest['solverVerification']['completionMode']))
for slot in [2,0,1]:
 commit(plans[slot][0],slot,int(plans[slot][1]));clear()
 assert map_value(-1,1)==sum(results[slot]['mapTotals'])
for slot in range(3):
 assert route_configure(slot,None,route_sha) and clock_configure(slot,None) and mini_configure(slot,None,mini_sha)
commit(normal);clear();assert C.string_at(romptr,0x300000)==pristine and l.sm_cpu_opcodes()==0
(root/'escape/objectives-native-results.json').write_text(json.dumps(dict(passed=True,seeds=results,sourceWsDecompression=True,filteredMapCounts=True,quotaTriggered=True,slotRevalidation=True,fullRomRestored=True,nativeSha256=hashlib.sha256((root/'escape/libsm_native.so').read_bytes()).hexdigest()),indent=2)+'\n')
l.sm_shutdown();print('ESCAPE_OBJECTIVES_NATIVE_PASS',len(results),flush=True)
