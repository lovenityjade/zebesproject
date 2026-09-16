#!/usr/bin/env python3
import ctypes as C,json,os,sys
from pathlib import Path
root=Path(sys.argv[1]);assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
out=root/'elevators-hud';out.mkdir(exist_ok=True)
lib=C.CDLL(str(root/'menu-options/libsm_native.so'));lib.sm_randomizer_generate.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
results=[]
for i,split in enumerate(['Full','Major','Chozo','FullWithHUD','FullWithHUD']):
 q=dict(seed=15092060+i,skill='regular',progression='medium',options=dict(majorsSplit=split,elevators_speed='on',minorQty=70,maxDifficulty='hard'))
 if i==4:q['relicHunt']=dict(enabled=True,placed=20,required=15)
 b=C.create_string_buffer(2097152);n=lib.sm_randomizer_generate(str(root/'Randomizer').encode(),json.dumps(q).encode(),b,len(b));assert 0<n<=len(b)
 r=json.loads(b.value);assert r['ok'],r;m=r['manifest'];(out/f'seed-{i}.json').write_text(json.dumps(r,indent=2))
 assert {'hud-counts-v1','fast-elevators-v1'}<=set(m['requiredNativeBehavior'])
 assert m['solverVerification']['allItemsReachable'] and m['solverVerification']['completionVerified']
 assert m['rules']['options']['elevators_speed']=='on' and m['nativeContext']['split']==split
 for p in m['placements']:
  assert type(p['hudCounted']) is bool
  if p['kind']==2:assert not p['hudCounted']
  if p['kind']==1:assert p['hudCounted']
 if split=='Full':assert sum(p['hudCounted'] for p in m['placements'])==sum(p['kind']!=2 for p in m['placements'])
 if split=='FullWithHUD' and i==3:assert sum(p['hudCounted'] for p in m['placements'])==16
 results.append(dict(seed=m['seed'],split=split,counted=sum(p['hudCounted'] for p in m['placements']),fingerprint=m['sha256']))
 print('ELEVATOR_HUD_SEED_PASS',i,results[-1],flush=True)
(out/'verification.json').write_text(json.dumps(results,indent=2))
