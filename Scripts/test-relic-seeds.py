import ctypes as C,json,os,sys
from pathlib import Path
root=Path(sys.argv[1]);assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').exists()
l=C.CDLL(str(root/'full-options/libsm_native.so'))
l.sm_randomizer_generate.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
results=[]
requests=[dict(seed=15092026+i,skill='casual',progression='medium',noAdvancedTechs=True,relicHunt=dict(enabled=True,placed=30,required=20)) for i in range(3)]
requests+=[dict(seed=15092030,skill='casual',progression='medium',noAdvancedTechs=True),dict(seed=15092031,options=dict(areaRandomization='full')),dict(seed=15092031,relicHunt=dict(enabled=True,placed=10,required=11))]
for i,q in enumerate(requests):
 b=C.create_string_buffer(2097152);n=l.sm_randomizer_generate(str(root/'Randomizer').encode(),json.dumps(q).encode(),b,len(b));assert 0<n<=len(b)
 r=json.loads(b.value)
 (root/f'full-options/verified-{i}.json').write_bytes(b.value)
 if i<4:
  assert r['ok'],r
  m=r['manifest'];v=m['solverVerification'];assert v['allItemsReachable'] and v['completionVerified']
  techs=sorted({t for p in v['progressionLog'] for t in p['techniques']})
  assert set(techs)<= {'WallJump','ShineSpark','MidAirMorph','CrouchJump','UnequipItem'},techs
  if i<3:
   assert sum(p['kind']==1 for p in m['placements'])==30
   assert v['relicCompletion']['collected']>=20 and v['relicCompletion']['returnVerified']
  results.append(dict(seed=m['seed'],checks=v['reachableItemCount'],techniques=techs,completion=v.get('relicCompletion'),sha256=m['sha256']))
 else:
  assert not r['ok'];results.append(dict(rejected=q,error=r['error']))
 print('SEED_CASE',i,'PASS',flush=True)
(root/'full-options/seed-verification.json').write_text(json.dumps(results,indent=2))
