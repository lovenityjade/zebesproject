#!/usr/bin/env python3
"""Public escape options, effective dependencies and full solver routes."""
import json,os,subprocess,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
out=root/'escape/public';out.mkdir(parents=True,exist_ok=True)
if len(sys.argv)==2:
 for i in range(4):subprocess.run([sys.executable,__file__,str(root),str(i)],check=True)
 raise SystemExit(0)
i=int(sys.argv[2]);sys.path[:0]=[str(root/'Randomizer'),str(root/'Randomizer/upstream')]
import sm_options,sm_integration
options=dict(maxDifficulty='hard',tourian='Vanilla' if i==0 else 'Disabled',escapeRando='on' if i==0 else 'off',removeEscapeEnemies='on')
if i==2:options.update(minimizer='on',minimizerQty=60)
if i==3:options.update(tourian='Fast',escapeRando='on',majorsSplit='Scavenger',scavNumLocs=4,areaRandomization='light')
result=json.loads(sm_integration.generate_json(json.dumps(dict(seed=15093500+i,skill='regular',options=options))))
assert result.get('ok'),result
m=result['manifest']
o=m['nativeContext']['objectives'];e=m['nativeContext']['topology']['nativeEscape'];v=m['solverVerification']
assert v['allItemsReachable'] and v['completionVerified'] and v['escapeToShip']
assert v['motherBrainDefeated']==(i in (0,3))
assert o['schema']==5 and o['escape']==e and bool(o['flags']&16)==(i in (1,2))
assert m['tracker']['settings']['nativeObjectives']==o and m['tracker']['topology']['nativeEscape']==e
assert m['rules']['options']['escapeRando']=='on'
assert m['rules']['options']['removeEscapeEnemies']==('off' if i in (1,2) else 'on')
assert 'native-escape-v1' in m['requiredNativeBehavior']
assert all(n in m['nativeContext']['world']['nativeCode'] for n in ['rando_escape_common.ips','rando_escape.ips','map_data_escape_rando.ips','rando_escape_ws_fix.ips'])
(out/f'seed-{i:02d}.json').write_text(json.dumps(dict(ok=True,manifest=m),indent=2)+'\n')
print('ESCAPE_PUBLIC_PASS',i,v['reachableItemCount'],m['sha256'],flush=True)
