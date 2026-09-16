#!/usr/bin/env python3
"""Replay the Easy/Ridley incident and protect progression ammo in tablet hunts."""
import ctypes as C,json,os,time,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').exists()
out=root/'relic-generation-regressions';out.mkdir(exist_ok=True)
lib=C.CDLL(str(root/'Native/build/libsm_native.so'))
lib.sm_randomizer_generate.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
q=dict(seed=752526497,skill='casual',progression='fast',patches=[],
       options=dict(maxDifficulty='easy',progressionDifficulty='easier'),
       noAdvancedTechs=True,relicHunt=dict(enabled=True,placed=30,required=5),
       skillSettings={'Kraid':"He's annoying",'Ridley':"I'm scared!",'MotherBrain':'It can get ugly'})
requests=[('reported',q)]
for i in range(3):
 r=json.loads(json.dumps(q));r['seed']+=i;r['skillSettings']['Ridley']='Default';requests.append((f'easy-default-ridley-{i}',r))
r=json.loads(json.dumps(q));r['options']['maxDifficulty']='hard';requests.append(('hard-original-combat',r))
requests.append(('ordinary-regression',dict(seed=1032809128,skill='casual',progression='medium')))
for name,request in requests:
 start=time.monotonic();buf=C.create_string_buffer(2097152)
 n=lib.sm_randomizer_generate(str(root/'Randomizer').encode(),json.dumps(request).encode(),buf,len(buf));assert 0<n<=len(buf)
 result=json.loads(buf.value);(out/(name+'.json')).write_text(json.dumps(result,indent=2))
 if name=='reported':
  assert not result['ok'] and not result.get('retry') and 'manifest' not in result
  assert 'Ridley' in result['error'] and 'Maximum difficulty / easy' in result['error']
  assert result['generationAttempts']==0 and result['settingsIssues'][0]['page']==8
 else:
  assert result['ok'],result
 if not result['ok']:
  print(name,'REJECTED',round(time.monotonic()-start,2),result['error'],flush=True)
 else:
  m=result['manifest'];v=m['solverVerification'];assert v['allItemsReachable'] and v['completionVerified']
  if request.get('relicHunt',{}).get('enabled'):
   assert sum(p['kind']==1 for p in m['placements'])==30
   protected={p['location']:p['item'] for p in m['generatorProgressionBeforeRelicReplacement']}
   assert all(p['item']==protected[p['location']] for p in m['placements'] if p['location'] in protected)
   assert v['relicCompletion']['returnVerified'] and v['relicCompletion']['collected']>=5
   assert {t for p in v['progressionLog'] for t in p['techniques']}<= {'WallJump','ShineSpark','MidAirMorph','CrouchJump','UnequipItem'}
  print(name,'VERIFIED',v['reachableItemCount'],m['generationAttempts'],round(time.monotonic()-start,2),flush=True)
