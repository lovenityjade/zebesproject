#!/usr/bin/env python3
"""Generate native door-color plans with fresh solver and effective topology."""
import ctypes as C,hashlib,json,os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
lib=C.CDLL(str(root/'door-colors/libsm_native.so'));lib.sm_randomizer_generate.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
out=root/'door-color-seeds';out.mkdir(exist_ok=True);reports=[]
for i in range(4):
 request=dict(seed=15092500+i,skill='regular',noAdvancedTechs=bool(i%2),options=dict(doorsColorsRando='on',allowGreyDoors='on' if i>=2 else 'off',layoutPatches='on',variaTweaks='on',bossRandomization='on' if i==3 else 'off'))
 b=C.create_string_buffer(2097152);n=lib.sm_randomizer_generate(str(root/'Randomizer').encode(),json.dumps(request).encode(),b,len(b));assert 0<n<=len(b)
 r=json.loads(b.value);assert r.get('ok'),r
 m=r['manifest'];v=m['solverVerification'];d=m['nativeContext']['doorColors']
 assert v['allItemsReachable'] and v['completionVerified'] and v['reachableItemCount']==100
 assert 'native-door-colors-v1' in m['requiredNativeBehavior'] and len(d['colors'])==58
 assert d['doors']==m['tracker']['topology']['doors']
 assert m['tracker']['topology']['doorColors']=={k:v for k,v in d.items() if k!='doors'}
 assert any(x>=5 for x in d['colors']) and (i>=2 or 4 not in d['colors'])
 assert any(0xf60b<=x['plm']<=0xf665 for x in m['nativeContext']['world']['indicators'])
 (out/f'seed-{i:02d}.json').write_text(json.dumps(r,indent=2)+'\n');reports.append(dict(seed=m['seed'],fingerprint=m['sha256'],colors=d['colors'],checks=v['reachableItemCount'],noAdvanced=m['rules']['noAdvancedTechs']))
 print('DOOR_COLOR_SEED_PASS',i,m['seed'],flush=True)
assert any(4 in r['colors'] for r in reports[2:]),'Grey-door pool was not exercised'
(out/'verification.json').write_text(json.dumps(dict(seeds=reports,nativeSha256=hashlib.sha256((root/'door-colors/libsm_native.so').read_bytes()).hexdigest()),indent=2)+'\n')
