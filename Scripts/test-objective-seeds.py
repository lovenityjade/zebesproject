#!/usr/bin/env python3
"""Fingerprint, tracker and native-plan publication for effective G4 objectives."""
import ctypes as C,hashlib,json,os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
l=C.CDLL(str(root/'objectives-binding/libsm_native.so'))
l.sm_randomizer_generate.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
out=root/'objective-seeds';out.mkdir(exist_ok=True)
requests=[
 dict(seed=15092800,skill='casual',options={}),
 dict(seed=15092801,skill='regular',options=dict(majorsSplit='FullWithHUD',areaRandomization='full',startLocation='Golden Four',layoutPatches='on',variaTweaks='on')),
 dict(seed=15092802,skill='regular',options=dict(majorsSplit='Chozo',bossRandomization='on',doorsColorsRando='on')),
 dict(seed=15092803,skill='casual',relicHunt=dict(enabled=True,placed=30,required=15))]
reports=[]
for i,request in enumerate(requests):
 b=C.create_string_buffer(2097152);n=l.sm_randomizer_generate(str(root/'Randomizer').encode(),json.dumps(request).encode(),b,len(b))
 assert 0<n<=len(b);result=json.loads(b.value)
 (out/f'candidate-{i:02d}.json').write_text(json.dumps(result,indent=2)+'\n')
 assert result.get('ok'),result
 m=result['manifest'];v=m['solverVerification'];ctx=m['nativeContext']
 assert v['allItemsReachable'] and v['completionVerified'] and v['reachableItemCount']==100
 canonical=json.dumps({k:m[k] for k in ('schema','seed','upstream','placements','patches','logicPatches','requiredNativeBehavior','rules','nativeContext','requestedSettings')},sort_keys=True,separators=(',',':')).encode()
 assert hashlib.sha256(canonical).hexdigest()==m['sha256']
 if i<3:
  obj=ctx['objectives'];assert 'native-objectives-v1' in m['requiredNativeBehavior']
  assert obj==m['tracker']['settings']['nativeObjectives']
  assert obj['names']==ctx['goals']==['kill kraid','kill phantoon','kill draygon','kill ridley']
  assert obj['required']==ctx['goalsRequired']==4 and obj['flags']==0
  assert obj['areaCounted']==[int(p['hudCounted']) for p in m['placements']]
  assert obj['itemCounted']==[int(not p['kind']) for p in m['placements']]
  assert obj['areaLayout']==(i==1)
 else:
  assert 'objectives' not in ctx and 'nativeObjectives' not in m['tracker']['settings']
  assert 'native-objectives-v1' not in m['requiredNativeBehavior']
  assert v['completionMode']=='chozo-relic-hunt' and v['relicCompletion']['required']==15
 (out/f'seed-{i:02d}.json').write_text(json.dumps(result,indent=2)+'\n')
 reports.append(dict(seed=m['seed'],fingerprint=m['sha256'],checks=100,split=ctx['split'],objectiveCount=len(ctx.get('objectives',{}).get('goals',[])),completionMode=v['completionMode']))
 print('OBJECTIVE_SEED_PASS',i,m['seed'],flush=True)
(out/'verification.json').write_text(json.dumps(dict(seeds=reports,nativeSha256=hashlib.sha256((root/'objectives-binding/libsm_native.so').read_bytes()).hexdigest()),indent=2)+'\n')
