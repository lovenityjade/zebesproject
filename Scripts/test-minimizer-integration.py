#!/usr/bin/env python3
"""Minimizer internal integration: source routes and filtered objective contracts. Public activation remains gated."""
import json,os,subprocess,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
out=root/'minimizer/integration';out.mkdir(parents=True,exist_ok=True)
cases=[dict(seed=15093300+i,options=dict(tourian='Fast',maxDifficulty='hard')) for i in range(3)]
if len(sys.argv)==2:
 for i in range(len(cases)):
  subprocess.run([sys.executable,__file__,str(root),str(i)],check=True)
 raise SystemExit(0)
i=int(sys.argv[2]);request=dict(cases[i],skill='regular')
sys.path[:0]=[str(root/'Randomizer'),str(root/'Randomizer/upstream')]
import sm_options
original_resolve=sm_options.resolve
def resolve(request):
 rules=original_resolve(request)
 rules['options'].update(minimizer='on',minimizerQty=[30,60,100][i],areaRandomization='full',bossRandomization='on')
 return rules
sm_options.resolve=resolve
from sm_integration import generate_json
# Fresh interpreter for every production attempt (upstream has global state).
if len(sys.argv)>3:
 result=json.loads(generate_json(json.dumps(request),int(sys.argv[3])))
 (out/f'candidate-{i}.json').write_text(json.dumps(result,indent=2)+'\n');raise SystemExit(0)
for attempt in range(8):
 subprocess.run([sys.executable,__file__,str(root),str(i),str(attempt)],check=True)
 result=json.loads((out/f'candidate-{i}.json').read_text())
 if result.get('ok'):break
 if not result.get('retry'):raise AssertionError(result)
assert result.get('ok'),result
m=result['manifest'];o=m['nativeContext']['objectives'];v=m['solverVerification']
assert v['allItemsReachable'] and v['completionVerified'] and v['reachableItemCount']==sum(m['nativeContext']['topology']['nativeMinimizer']['checks'])
assert o['schema']==4 and o['tourian']=='Fast' and o['flags']&8
assert m['nativeContext']['tourian']=='Fast' and 'native-fast-tourian-v1' in m['requiredNativeBehavior']
assert m['tracker']['settings']['nativeObjectives']==o and m['tracker']['settings']['tourian']=='Fast'
(out/f'seed-{i:02d}.json').write_text(json.dumps(result,indent=2)+'\n')
print('MINIMIZER_INTEGRATION_PASS',i,m['sha256'],flush=True)
