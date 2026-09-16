#!/usr/bin/env python3
"""Public Animals Surprise generation, source progression and embedded tracker."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
fixture=(root/'test-live-tracker-logic.py').read_text().split('v=evaluate(')[0]
fixture=fixture.replace("out=root/'tracker-results'","out=root/'animals/public'").replace("root/'Native/build/libsm_native.so'","root/'animals/libsm_native.so'")
exec(compile(fixture,'animals-tracker-fixture','exec'))
generate=lib.sm_randomizer_generate;generate.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
reports=[]
for i in range(3):
 options={'animals':'on'}
 if i==2:options['escapeRando']='on'
 b=C.create_string_buffer(2097152)
 n=generate(str(root/'Randomizer').encode(),json.dumps(dict(seed=15093900+i,skill='regular',options=options)).encode(),b,len(b))
 assert 0<n<=len(b);result=json.loads(b.value);assert result.get('ok'),result
 (out/f'seed-{i:02d}.json').write_text(json.dumps(result,indent=2)+'\n')
 m=result['manifest'];v=m['solverVerification'];t=m['tracker'];steps=[];visited=set()
 assert v['allItemsReachable'] and v['completionVerified'] and v['reachableItemCount']==100
 assert ('native-animals-v1' in m['requiredNativeBehavior'])==(i!=2)
 assert m['rules']['options']['animals']==('off' if i==2 else 'on')
 if i==2:assert 'animals' not in m['nativeContext'] and any('Animals Surprise ignored' in x for x in m['rules']['adjustments'])
 else:
  contract=m['nativeContext']['animals'];assert 1<=contract['mode']<=10
  placements={p['location']:p['item'] for p in m['placements']}
  for p in v['progressionLog']:
   if p['location'] in placements:
    assert p['item']==placements[p['location']]
    r=evaluate(request(t,from_inventory(t,p['inventoryBefore'],visited)))
    c=next(c for c in r['checks'] if c['name']==p['location'])
    assert c['reachable'] and c['state'] in (1,4)
    steps.append(dict(step=p['step'],location=p['location'],state=c['state']))
   visited.add(p['location'])
  assert len(steps)==100
 reports.append(dict(seed=m['seed'],sha256=m['sha256'],animals=m['nativeContext'].get('animals'),steps=steps))
 print('ANIMALS_SEED_PASS',m['seed'],m['nativeContext'].get('animals'),len(steps),flush=True)
(out/'verification.json').write_text(json.dumps(dict(seeds=reports,queries=queries,seconds=seconds),indent=2)+'\n')
