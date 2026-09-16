#!/usr/bin/env python3
"""Retained Minimizer progression checks and excluded marker states, embedded oracle."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-live-tracker-logic.py').read_text().split('v=evaluate(')[0]
source=source.replace("out=root/'tracker-results'", "out=root/'minimizer/tracker'")
source=source.replace("root/'Native/build/libsm_native.so'","root/'minimizer/libsm_native.so'")
exec(compile(source,'objective-tracker-fixture','exec'))
reports=[]
for path in sorted((root/'minimizer/integration').glob('seed-*.json')):
 m=json.loads(path.read_text())['manifest'];t=m['tracker'];visited=set();cases=[]
 for p in m['solverVerification']['progressionLog']:
  inv=from_inventory(t,p['inventoryBefore'],visited)
  r=evaluate(request(t,inv));byname={c['name']:c for c in r['checks']}
  assert [c['enabled'] for c in r['checks']]==list(map(bool,t['topology']['nativeMinimizer']['checks']))
  assert all(c['state']==0 and not c['reachable'] for c in r['checks'] if not c['enabled'])
  obj=r['objectives'];assert [g['name'] for g in obj['goals']]==t['settings']['nativeObjectives']['names']
  assert obj['required']==t['settings']['nativeObjectives']['required']
  assert obj['progressSource']=='unavailable' and all(g['completed'] is None for g in obj['goals'])
  if p['location'] in byname:
   c=byname[p['location']];assert c['reachable'] and c['state'] in (1,4),(m['seed'],p['step'],c)
   assert all(c['state']==0 for c in r['checks'] if c['name'] in visited)
   cases.append(dict(step=p['step'],location=p['location'],state=c['state']))
  visited.add(p['location'])
 # Re-evaluate the inventory at each actual source objective step. This checks
 # region memberships/percentage thresholds and the bounded goal predicates.
 for entry in m['solverVerification']['objectiveVerification']['progressionLog']:
  inv=from_inventory(t,entry['inventoryBefore'],set(entry['collectedChecksBefore']))
  r=evaluate(request(t,inv));g=next(g for g in r['objectives']['goals'] if g['name']==entry['objectiveName'])
  assert g['completable'],(m['seed'],entry,g)
 assert len(cases)==sum(t['topology']['nativeMinimizer']['checks'])
 reports.append(dict(seed=m['seed'],fingerprint=m['sha256'],cases=cases))
 print('MINIMIZER_TRACKER_PASS',m['seed'],len(cases),flush=True)
assert len(reports)==3
(out/'verification.json').write_text(json.dumps(dict(passed=True,seeds=reports,queries=queries,querySeconds=seconds,scope=__doc__),indent=2)+'\n')
