#!/usr/bin/env python3
"""Public Scavenger requests: serialized placements, ordered solver and tracker replay."""
import json,os,subprocess,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
out=root/'scavenger/public';out.mkdir(parents=True,exist_ok=True)
cases=[dict(seed=15093100,options=dict(majorsSplit='Scavenger',scavNumLocs=4,scavRandomized='off')),
 dict(seed=15093101,options=dict(majorsSplit='Scavenger',scavNumLocs=8,scavRandomized='on',areaRandomization='full')),
 dict(seed=15093102,options=dict(majorsSplit='Scavenger',scavNumLocs=17,scavRandomized='off',bossRandomization='on',maxDifficulty='infinity'))]
if len(sys.argv)==2:
 for i in range(len(cases)):
  subprocess.run([sys.executable,__file__,str(root),str(i)],check=True)
 raise SystemExit(0)
i=int(sys.argv[2]);request=dict(cases[i],skill='regular')
sys.path[:0]=[str(root/'Randomizer'),str(root/'Randomizer/upstream')]
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
m=result['manifest'];h=m['nativeContext']['scavenger'];v=m['solverVerification']
assert v['allItemsReachable'] and v['completionVerified'] and v['reachableItemCount']==100
assert v['scavengerVerification']==dict(order=h['locations'],ordered=True)
assert m['tracker']['settings']['scavenger']==h==m['nativeContext']['objectives']['scavenger']
assert m['nativeContext']['objectives']['schema']==2 and 16 in m['nativeContext']['objectives']['goals']
(out/f'seed-{i:02d}.json').write_text(json.dumps(result,indent=2)+'\n')
print('SCAVENGER_PUBLIC_PASS',i,len(h['words']),m['sha256'],flush=True)
