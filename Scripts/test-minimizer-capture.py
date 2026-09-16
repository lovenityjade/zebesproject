#!/usr/bin/env python3
"""Effective source Minimizer worlds; capture before public native activation."""
import copy,json,os,subprocess,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
out=root/'minimizer/capture';out.mkdir(parents=True,exist_ok=True)
if len(sys.argv)==2:
 for i in range(3):subprocess.run([sys.executable,__file__,str(root),str(i)],check=True)
 raise SystemExit(0)
i=int(sys.argv[2]);count=[30,60,100][i]
sys.path[:0]=[str(root/'Randomizer'),str(root/'Randomizer/upstream')]
import sm_options,sm_native_world,sm_integration,sm_minimizer
old=sm_options.resolve
def resolve(request):
 rules=old(request)
 rules['options'].update(minimizer='on',minimizerQty=count,areaRandomization='full',bossRandomization='on')
 return rules
class Done(BaseException):pass
def capture(patcher,rules):
 contract=sm_minimizer.capture(patcher);c=sm_minimizer.catalog();d=contract['destinations']
 assert all(d[target]==source for source,target in enumerate(d))
 mixed=[(c['accessPoints'][a]['name'],c['accessPoints'][b]['name']) for a,b in enumerate(d) if (a<32)!=(b<32)]
 assert mixed
 (out/f'seed-{i:02d}.json').write_text(json.dumps(dict(seed=15093300+i,requestedCount=count,contract=contract,mixedPairs=mixed,
    sourceNonBossLocations=sum(not x.Location.isBoss() for x in patcher.settings['itemLocs']),
    sourceRestricted=[dict(name=x.Location.Name,item=x.Item.Type,area=x.Location.GraphArea) for x in patcher.settings['itemLocs'] if x.Location.restricted]),indent=2)+'\n')
 print('MINIMIZER_CAPTURE_PASS',count,len(contract['locations']),flush=True);raise Done()
sm_options.resolve=resolve;sm_native_world.capture=capture
try:sm_integration.generate_once(dict(seed=15093300+i,skill='regular',options=dict(tourian='Fast')))
except Done:pass
