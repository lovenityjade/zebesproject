#!/usr/bin/env python3
"""Multiple actual generated tweak selections and their fresh solver routes."""
import ctypes as C,json,os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
out=root/'tweak-seeds';out.mkdir(exist_ok=True)
l=C.CDLL(str(root/'world-data/libsm_native.so'));l.sm_randomizer_generate.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
sys.path.insert(0,str(root/'Randomizer/upstream'))
from rom.rom_patches import RomPatches
cases=[[],['bomb_torizo'],['LN_Chozo'],['WS_Etank','LN_Chozo','bomb_torizo']]
results=[]
for i,custom in enumerate(cases):
 q=dict(seed=15092200+i,skill='regular',options=dict(variaTweaks='on' if custom else 'off',variaTweaksCustom=custom))
 b=C.create_string_buffer(2097152);n=l.sm_randomizer_generate(str(root/'Randomizer').encode(),json.dumps(q).encode(),b,len(b));assert 0<n<=len(b)
 r=json.loads(b.value);(out/f'seed-{i:02}.json').write_text(json.dumps(r,indent=2));assert r['ok'],r
 m=r['manifest'];v=m['solverVerification'];world=m['nativeContext']['world']
 assert v['allItemsReachable'] and v['completionVerified'] and v['reachableItemCount']==100
 for name,patch,logic in [('bomb_torizo','bomb_torizo.ips',RomPatches.BombTorizoWake),('LN_Chozo','LN_Chozo_SpaceJump_Check_Disable',RomPatches.LNChozoSJCheckDisabled),('WS_Etank','WS_Etank',RomPatches.WsEtankPhantoonAlive)]:
  assert (patch in world['dataPatches'])==(name in custom)
  assert (logic in m['logicPatches'])==(name in custom)
  assert (logic in m['tracker']['settings']['logicPatches'])==(name in custom)
 byname={p['location']:p for p in m['placements']}
 for step in v['progressionLog']:
  if step['location'] in byname:assert step['item']==byname[step['location']]['item']
 results.append(dict(seed=m['seed'],selection=custom,checks=v['reachableItemCount'],world=world,logicPatches=m['logicPatches']))
 print('TWEAK_SEED_PASS',i,m['seed'],flush=True)
(out/'verification.json').write_text(json.dumps(results,indent=2))
