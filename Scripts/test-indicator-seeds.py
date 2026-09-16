#!/usr/bin/env python3
import ctypes as C,json,os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
out=root/'indicator-seeds';out.mkdir(exist_ok=True)
l=C.CDLL(str(root/'world-data/libsm_native.so'));l.sm_randomizer_generate.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
cases=[dict(layoutPatches='off'),dict(layoutPatches='on',layoutCustom=['door_indicators_plms']),dict(layoutPatches='on'),dict(layoutPatches='on',variaTweaks='on')]
results=[]
for i,options in enumerate(cases):
 q=dict(seed=15092300+i,skill='regular',options=options)
 b=C.create_string_buffer(2097152);n=l.sm_randomizer_generate(str(root/'Randomizer').encode(),json.dumps(q).encode(),b,len(b));assert 0<n<=len(b)
 r=json.loads(b.value);(out/f'seed-{i:02}.json').write_text(json.dumps(r,indent=2));assert r['ok'],r
 m=r['manifest'];v=m['solverVerification'];w=m['nativeContext']['world']
 assert v['allItemsReachable'] and v['completionVerified'] and v['reachableItemCount']==100
 assert ('native-door-indicators-v1' in m['requiredNativeBehavior'])==(i!=0)
 if i:assert len(w['indicators'])==13 and len({e['locationId'] for e in w['indicators']})==13
 byname={p['location']:p for p in m['placements']}
 for step in v['progressionLog']:
  if step['location'] in byname:assert step['item']==byname[step['location']]['item']
 results.append(dict(seed=m['seed'],world=w,checks=v['reachableItemCount']))
 print('INDICATOR_SEED_PASS',i,flush=True)
(out/'verification.json').write_text(json.dumps(results,indent=2))
