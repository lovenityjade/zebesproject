#!/usr/bin/env python3
"""Public native generation and embedded tracker progression for gameplay patches."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
fixture=(root/'test-live-tracker-logic.py').read_text().split('v=evaluate(')[0]
fixture=fixture.replace("out=root/'tracker-results'","out=root/'movement-pickups/public'").replace("root/'Native/build/libsm_native.so'","root/'movement-pickups/libsm_native.so'")
exec(compile(fixture,'gameplay-tracker-fixture','exec'))
generate=lib.sm_randomizer_generate;generate.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
sys.path.insert(0,str(root/'Randomizer/upstream'))
from rom.rom_patches import RomPatches
reports=[]
keys=[('spinjumprestart','respin-v1'),('Infinite_Space_Jump','infinite-spacejump-v1'),('itemsounds','item-sounds-v1')]
for i in range(2):
 options={key:'on' if i==0 or key=='Infinite_Space_Jump' else 'off' for key,_ in keys}
 q=dict(seed=15093800+i,skill='regular',options=options)
 b=C.create_string_buffer(2097152);n=generate(str(root/'Randomizer').encode(),json.dumps(q).encode(),b,len(b));assert 0<n<=len(b)
 result=json.loads(b.value);assert result.get('ok'),result
 (out/f'seed-{i:02d}.json').write_text(json.dumps(result,indent=2)+'\n')
 m=result['manifest'];v=m['solverVerification'];t=m['tracker'];visited=set();steps=[]
 assert v['allItemsReachable'] and v['completionVerified'] and v['reachableItemCount']==100
 for key,behavior in keys:
  assert m['rules']['options'][key]==options[key]
  assert (behavior in m['requiredNativeBehavior'])==(options[key]=='on')
 assert m['logicPatches']==t['settings']['logicPatches']
 placements={p['location']:p['item'] for p in m['placements']}
 for p in v['progressionLog']:
  if p['location'] in placements:
   assert p['item']==placements[p['location']]
   r=evaluate(request(t,from_inventory(t,p['inventoryBefore'],visited)))
   c=next(c for c in r['checks'] if c['name']==p['location'])
   assert c['reachable'] and c['state'] in (1,4),(m['seed'],p['step'],c)
   steps.append(dict(step=p['step'],location=p['location'],state=c['state']))
  visited.add(p['location'])
 assert len(steps)==100
 reports.append(dict(seed=m['seed'],sha256=m['sha256'],options=options,logicPatches=m['logicPatches'],steps=steps))
 print('MOVEMENT_PICKUP_SEED_TRACKER_PASS',m['seed'],len(steps),flush=True)
(out/'verification.json').write_text(json.dumps(dict(seeds=reports,queries=queries,seconds=seconds),indent=2)+'\n')
