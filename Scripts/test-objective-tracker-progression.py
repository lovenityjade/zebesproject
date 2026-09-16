#!/usr/bin/env python3
"""Every check in four objective-contract/relic seed progression logs, embedded oracle.
Inventory/boss flags are solver fixtures; no claim of physical traversal.
"""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-live-tracker-logic.py').read_text().split('v=evaluate(')[0]
source=source.replace("out=root/'tracker-results'", "out=root/'objectives-binding/tracker'")
source=source.replace("root/'Native/build/libsm_native.so'","root/'objectives-binding/libsm_native.so'")
exec(compile(source,'area-tracker-fixture','exec'))
reports=[]
for path in sorted((root/'objective-seeds').glob('seed-*.json')):
 m=json.loads(path.read_text())['manifest'];t=m['tracker'];visited=set();cases=[]
 assert len(t['settings']['initialDoors'])>=9
 for p in m['solverVerification']['progressionLog']:
  inv=from_inventory(t,p['inventoryBefore'],visited)
  r=evaluate(request(t,inv));byname={c['name']:c for c in r['checks']}
  assert r['topologyMode']==t['topology']['mode'] and r['bossConnections']==t['topology']['bossPairs']
  if p['location'] in byname:
   c=byname[p['location']]
   assert c['reachable'] and c['state'] in (1,4),(m['seed'],p['step'],p['location'],c)
   assert all(c['state']==0 for c in r['checks'] if c['name'] in visited)
   cases.append(dict(step=p['step'],location=p['location'],state=c['state'],reason=c['reason']))
  visited.add(p['location'])
 assert len(cases)==100
 reports.append(dict(seed=m['seed'],fingerprint=m['sha256'],noAdvancedTechs=m['rules']['noAdvancedTechs'],cases=cases))
 print('OBJECTIVE_TRACKER_PROGRESSION_PASS',m['seed'],len(cases),flush=True)
assert len(reports)==4
(out/'verification.json').write_text(json.dumps(dict(passed=True,seeds=reports,queries=queries,querySeconds=seconds,scope=__doc__),indent=2)+'\n')
