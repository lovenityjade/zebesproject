#!/usr/bin/env python3
"""Source-generated Scavenger orders; fixture-only access before public binding."""
import copy,json,os,subprocess,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
out=root/'scavenger/capture';out.mkdir(parents=True,exist_ok=True)
cases=[dict(seed=15093100,count=4,randomized='off',options={}),
       dict(seed=15093101,count=8,randomized='on',options={'areaRandomization':'full'}),
       dict(seed=15093102,count=17,randomized='off',options={'bossRandomization':'on','maxDifficulty':'infinity'})]
if len(sys.argv)==2:
    reports=[]
    for i in range(len(cases)):
        for attempt in range(8):
            subprocess.run([sys.executable,__file__,str(root),str(i),str(attempt)],check=True)
            if (out/f'case-{i}.json').exists():break
        assert (out/f'case-{i}.json').exists(),('source exhausted retries',i)
        reports.append(json.loads((out/f'case-{i}.json').read_text()))
    (out/'verification.json').write_text(json.dumps(dict(passed=True,cases=reports,scope=__doc__),indent=2)+'\n')
    raise SystemExit(0)
i=int(sys.argv[2]);case=copy.deepcopy(cases[i]);case['seed']+=int(sys.argv[3]) if len(sys.argv)>3 else 0
(out/f'case-{i}.json').unlink(missing_ok=True)
sys.path[:0]=[str(root/'Randomizer'),str(root/'Randomizer/upstream')]
import sm_integration,sm_options,sm_native_world,sm_scavenger
old_resolve=sm_options.resolve
def resolve(request):
    rules=old_resolve(request)
    rules['options'].update(majorsSplit='Scavenger',scavNumLocs=case['count'],scavRandomized=case['randomized'])
    return rules
class Captured(BaseException):pass
def capture(patcher,rules):
    before=copy.deepcopy(patcher.romFile.data)
    contract=sm_scavenger.capture(patcher)
    assert patcher.romFile.data==before
    assert len(contract['words'])==case['count']
    assert contract['locations']==[il.Location.Name for il in patcher.settings['progItemLocs']]
    assert contract['words']==[il.Location.Id<<8|il.Location.HUD for il in patcher.settings['progItemLocs']]
    for kind in range(5):
        wrong=copy.deepcopy(contract)
        if kind==0:wrong['words'][0]=0xffff
        elif kind==1:wrong['words'][1]=wrong['words'][0]
        elif kind==2:wrong['locations'].reverse()
        elif kind==3:wrong['catalogSha256']='bad'
        else:wrong['words']=[];wrong['locations']=[]
        try:sm_scavenger.validate(wrong)
        except ValueError:pass
        else:raise AssertionError(('accepted malformed order',kind))
    report=dict(request=case,contract=contract,sourceHud=patcher.settings['hud'],
        effectiveGoals=rules['options']['objective'],sourceItemLocations=len(patcher.settings['itemLocs']),
        note='Source order capture only; not native activation, full solver proof or gameplay')
    (out/f'case-{i}.json').write_text(json.dumps(report,indent=2)+'\n')
    print('SCAVENGER_CAPTURE_PASS',case['seed'],contract['locations'],flush=True)
    raise Captured()
sm_options.resolve=resolve;sm_native_world.capture=capture
try:sm_integration.generate_once(dict(seed=case['seed'],skill='regular',options=case['options']))
except Captured:pass
except ValueError as error:
    if 'timed out' not in str(error):raise
    (out/f'timeout-{i}-{case["seed"]}.txt').write_text(str(error)+'\n')
    print('SCAVENGER_SOURCE_TIMEOUT',case['seed'],flush=True)
else:raise AssertionError('Source order capture was not reached')
