#!/usr/bin/env python3
"""Effective Minimizer dependencies without altering the saved user request."""
import copy,json,os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
sys.path[:0]=[str(root/'Randomizer'),str(root/'Randomizer/upstream')]
from sm_options import resolve
cases=[]
for seed in range(20):
 request=dict(seed=seed,options=dict(minimizer='on',minimizerQty='random',areaRandomization='off',bossRandomization='off',suitsRestriction='on',majorsSplit='Chozo'))
 original=copy.deepcopy(request);r=resolve(request);o=r['options']
 assert request==original and 30<=o['minimizerQty']<=60
 assert (o['areaRandomization'],o['bossRandomization'],o['suitsRestriction'],o['majorsSplit'])==('full','on','off','Full')
 assert r==resolve(request)
 cases.append(o['minimizerQty'])
for split in ['FullWithHUD','Scavenger','Chozo']:
 r=resolve(dict(seed=20,options=dict(minimizer='on',minimizerQty=100,majorsSplit=split)))
 assert r['options']['majorsSplit']==split
(root/'minimizer/settings-results.json').write_text(json.dumps(dict(passed=True,randomTargets=cases,preserved100Splits=['FullWithHUD','Scavenger','Chozo'],requestedSettingsUnchanged=True),indent=2)+'\n')
print('MINIMIZER_SETTINGS_PASS',len(cases)+3)
