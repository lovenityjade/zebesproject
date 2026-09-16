#!/usr/bin/env python3
"""Filtered VARIA objective writer versus original native pixels, including omitted spots."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
s=(root/'test-objective-map-native.py').read_text().split("\nfor goal in audit['goals']:")[0]
s=s.replace('objectives-map/libsm_native.so','minimizer/libsm_native.so').replace("root/'objectives-map/native'","root/'minimizer/icons'").replace('patcher._accessibleAreasNoBoss=set(graphAreas)','patcher._accessibleAreasNoBoss=set(retained)')
exec(compile(s,'minimizer-icons-fixture','exec'))
class Mini(C.Structure):
 _fields_=[('version',C.c_uint32),('size',C.c_uint32),('regions',C.c_uint16),('destinations',C.c_uint8*40),('checks',C.c_uint8*100),('reserved',C.c_uint16)]
l.sm_minimizer_configure.argtypes=[C.c_int,C.POINTER(Mini),C.c_char_p]
ids=[byname[n] for n in ['kill all space pirates','kill all beetoms']]
retained=graphAreas;full_rows=writer(ids)
for seed in range(3):
 m=json.loads((root/f'minimizer/public/seed-{seed:02d}.json').read_text())['manifest'];mini=m['nativeContext']['topology']['nativeMinimizer'];obj=m['nativeContext']['objectives']
 p=Mini();p.version=1;p.size=C.sizeof(p);p.regions=sum(1<<r for r in mini['regions']);p.destinations[:]=mini['destinations'];p.checks[:]=mini['checks']
 assert l.sm_minimizer_configure(0,C.byref(p),mini['catalogSha256'].encode())
 o=plan(ids,layout=1,flags=1|8);o.version=4
 for key,field in [('itemCounted','item_counted'),('areaCounted','area_counted'),('enemyTotals','enemy_totals'),('mapTotals','map_totals')]:getattr(o,field)[:]=obj[key]
 commit(o,0,1);clear();put(0x998,15);put(0x763,0)
 retained=[graphAreas[r] for r in mini['regions']];rows=writer(ids)
 if seed<2:assert len(rows)<len(full_rows)
 for row in full_rows:center(row);verify((seed,'retained-or-omitted',row),rows)
assert l.sm_cpu_opcodes()==0
(root/'minimizer/icon-results.json').write_text(json.dumps(dict(passed=True,pixelCases=cases,source='RomPatcher.writeObjectivesMapIcons with effective accessible regions',nativeSha256=hashlib.sha256((root/'minimizer/libsm_native.so').read_bytes()).hexdigest()),indent=2)+'\n')
l.sm_shutdown();print('MINIMIZER_FILTERED_ICONS_PASS',cases,flush=True)
