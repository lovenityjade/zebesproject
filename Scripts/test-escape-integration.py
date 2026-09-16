#!/usr/bin/env python3
"""Full generator/solver capture for the still-guarded escape integration."""
import json,os,subprocess,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
out=root/'escape/integration';out.mkdir(parents=True,exist_ok=True)
if len(sys.argv)==2:
 for i in range(3):subprocess.run([sys.executable,__file__,str(root),str(i)],check=True)
 raise SystemExit(0)
i=int(sys.argv[2]);sys.path[:0]=[str(root/'Randomizer'),str(root/'Randomizer/upstream')]
import sm_options,sm_integration
original=sm_options.resolve
def resolve(request):
 r=original(request);o=r['options'];o.update(tourian='Vanilla' if i==0 else 'Disabled',escapeRando='on',removeEscapeEnemies='on' if i==0 else 'off')
 if i==2:o.update(minimizer='on',minimizerQty=60,areaRandomization='full',bossRandomization='on')
 return r
sm_options.resolve=resolve
result=json.loads(sm_integration.generate_json(json.dumps(dict(seed=15093500+i,skill='regular',options=dict(maxDifficulty='hard')))))
assert result.get('ok'),result
m=result['manifest']
o=m['nativeContext']['objectives'];e=m['nativeContext']['topology']['nativeEscape'];v=m['solverVerification']
assert v['allItemsReachable'] and v['completionVerified'] and v['escapeToShip']
assert v['motherBrainDefeated']==(i==0)
assert o['schema']==5 and o['escape']==e and bool(o['flags']&16)==(i!=0)
assert m['tracker']['settings']['nativeObjectives']==o and m['tracker']['topology']['nativeEscape']==e
assert 'native-escape-v1' in m['requiredNativeBehavior']
assert all(n in m['nativeContext']['world']['nativeCode'] for n in ['rando_escape_common.ips','rando_escape.ips','map_data_escape_rando.ips','rando_escape_ws_fix.ips'])
(out/f'seed-{i:02d}.json').write_text(json.dumps(dict(ok=True,manifest=m),indent=2)+'\n')
print('ESCAPE_INTEGRATION_PASS',i,v['reachableItemCount'],m['sha256'],flush=True)
