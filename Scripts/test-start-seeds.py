#!/usr/bin/env python3
"""Actual in-process generation of varied native-start seeds, gaming-pc only."""
import ctypes as C,json,os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
out=root/'start-seeds';out.mkdir(exist_ok=True)
l=C.CDLL(str(root/'world-data/libsm_native.so'));l.sm_randomizer_generate.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
audit=json.loads((root/'world-data/dependencies.json').read_text());results=[]
for i,start in enumerate(audit['starts']):
 if start['start'].get('areaMode'):continue
 q=dict(seed=15092100+i,skill='solution',progression='medium',options=dict(startLocation=start['name'],maxDifficulty='infinity'))
 if start['name']=='Green Brinstar Elevator':q['options'].update(majorsSplit='Major',hud='off')
 if start['name']=='Gauntlet Top':q['options']['morphPlacement']='normal'
 b=C.create_string_buffer(2097152);n=l.sm_randomizer_generate(str(root/'Randomizer').encode(),json.dumps(q).encode(),b,len(b));assert 0<n<=len(b)
 r=json.loads(b.value);(out/f'seed-{i:02}.json').write_text(json.dumps(r,indent=2));assert r['ok'],(start['name'],r)
 m=r['manifest'];v=m['solverVerification'];world=m['nativeContext']['world']
 assert v['allItemsReachable'] and v['completionVerified']
 assert 'native-start-v1' in m['requiredNativeBehavior'] and world['startSpawn']==start['start']['spawn']
 assert m['rules']['options']['startLocation']==start['name'] and m['tracker']['settings']['start']==start['name']
 if start['name']=='Green Brinstar Elevator':assert m['rules']['options']['hud']=='on'
 if start['name']=='Gauntlet Top':assert m['rules']['options']['morphPlacement']=='early'
 byname={p['location']:p for p in m['placements']}
 for step in v['progressionLog']:
  if step['location'] in byname:assert step['item']==byname[step['location']]['item']
 results.append(dict(start=start['name'],seed=m['seed'],attempts=m['generationAttempts'],checks=v['reachableItemCount'],world=world,adjustments=m['rules']['adjustments']))
 print('START_SEED_PASS',start['name'],m['seed'],flush=True)
(out/'verification.json').write_text(json.dumps(results,indent=2))
