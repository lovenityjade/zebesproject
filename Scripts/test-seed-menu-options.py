#!/usr/bin/env python3
"""gaming-pc only: native ABI, resolved options, fresh solver and tracker parity."""
import ctypes as C,json,os,sys,hashlib
from pathlib import Path
root=Path(sys.argv[1]);assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
out=root/'menu-options';out.mkdir(exist_ok=True)
lib=C.CDLL(str(root/'menu-options/libsm_native.so'))
lib.sm_randomizer_generate.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
requests=[
 dict(seed=15092050,skill='casual',progression='medium',options=dict(missileQty=2.5,superQty=3,powerBombQty=1,minorQty=90,refill_before_save='on'),techniques={'Mockball':[False,5]},skillSettings={'Ice':'No thanks','Kraid':'Quick Kill'}),
 dict(seed=15092051,skill='regular',progression='fast',options=dict(majorsSplit='Major',maxDifficulty='hard',morphPlacement='normal',progressionDifficulty='easier')),
 dict(seed=15092052,skill='veteran',progression='slow',options=dict(progressionSpeed='random',progressionSpeedMultiSelect=['slow','fast'],energyQty='random',energyQtyMultiSelect=['vanilla','medium'],missileQty='random',maxDifficulty='random',majorsSplit='random',majorsSplitMultiSelect=['Full','Chozo'])),
 dict(seed=15092053,skill='expert',progression='speedrun',options=dict(progressionDifficulty='harder',funCombat='on',minorQty=80,maxDifficulty='hardcore')),
]
requests.append(dict(seed=15092055,skill='casual',progression='medium',options=dict(hideItems='on',hud='off',revealMap='off',better_reserves='off')))
requests.append(dict(seed=15092054,skill='expert',progression='fast',options=dict(energyQty='ultra sparse',fast_doors='on',maxDifficulty='infinity')))
results=[]
def generate(q):
 b=C.create_string_buffer(2097152);n=lib.sm_randomizer_generate(str(root/'Randomizer').encode(),json.dumps(q).encode(),b,len(b));assert 0<n<=len(b);return json.loads(b.value)
for i,q in enumerate(requests):
 r=generate(q);(out/f'seed-{i}.json').write_text(json.dumps(r,indent=2));assert r['ok'],r
 m=r['manifest'];v=m['solverVerification'];t=m['tracker']['settings'];assert v['allItemsReachable'] and v['completionVerified']
 for k in ('options','techniques','skillSettings'):assert m['requestedSettings'][k]==q.get(k,{})
 assert t['maximumDifficulty']==v['maximumDifficulty']
 assert t['progression']==m['rules']['options']['progressionSpeed']
 if i==4:
  assert 'hidden-items-v1' in m['requiredNativeBehavior']
  assert any(p['visibility']=='Hidden' and p['originalVisibility']=='Visible' for p in m['tracker']['locations'])
 assert t['preset']==m['rules']['preset']
 for k,values in q['options'].items():
  if k.endswith('MultiSelect') and q['options'].get(k[:-11])=='random':assert m['rules']['options'][k[:-11]] in values
 assert m['rules']['options']['maxDifficulty']!='random'
 if i==0:assert not t['knows']['Mockball']['enabled'] and 'save-refill-v1' in m['requiredNativeBehavior']
 if i==5:assert {'fast-doors-v1','nerfed-rainbow-v1'}<=set(m['requiredNativeBehavior'])
 if i==3:assert m['rules']['options']['progressionDifficulty']=='normal' and m['rules']['adjustments']
 by_name={p['location']:p for p in m['placements']}
 for step in v['progressionLog']:
  if step['location'] in by_name:assert step['item']==by_name[step['location']]['item']
 results.append(dict(seed=m['seed'],fingerprint=m['sha256'],checks=v['reachableItemCount'],steps=len(v['progressionLog']),attempts=m['generationAttempts'],effective=m['rules']['options'],adjustments=m['rules']['adjustments']))
 print('FULL_OPTIONS_SEED',i,'PASS',flush=True)
repeat=generate(requests[0]);assert repeat['ok'] and repeat['manifest']['sha256']==results[0]['fingerprint']
for change in [dict(areaRandomization='full'),dict(progressionSpeed='random',progressionSpeedMultiSelect=[]),dict(layoutCustom=['fake_patch']),dict(missileQty=0)]:
 q=dict(seed=15092054,options=change);r=generate(q);assert not r['ok'],change;results.append(dict(rejected=change,error=r['error']));print('REJECT',change,r['error'],flush=True)
(out/'verification.json').write_text(json.dumps(results,indent=2));print('FULL_OPTIONS_SEEDS_PASS deterministic=1 fresh_solver=1 tracker=1',flush=True)
