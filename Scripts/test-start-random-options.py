#!/usr/bin/env python3
import ctypes as C,json,os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
l=C.CDLL(str(root/'world-data/libsm_native.so'));l.sm_randomizer_generate.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
def generate(q):
 b=C.create_string_buffer(2097152);n=l.sm_randomizer_generate(str(root/'Randomizer').encode(),json.dumps(q).encode(),b,len(b));assert 0<n<=len(b);return json.loads(b.value)
results=[]
for seed in (15092130,15092131,15092132):
 q=dict(seed=seed,skill='casual',options=dict(startLocation='random',startLocationMultiSelect=['Landing Site','Gauntlet Top','Golden Four']))
 r=generate(q);assert r['ok'],r;m=r['manifest'];assert m['nativeContext']['start'] in ('Landing Site','Gauntlet Top')
 assert m['rules']['options']['startLocation']==m['nativeContext']['start']
 assert generate(q)['manifest']['sha256']==m['sha256']
 results.append(dict(seed=seed,start=m['nativeContext']['start'],fingerprint=m['sha256']))
 print('RANDOM_START_PASS',results[-1],flush=True)
for options in [dict(startLocation='random',startLocationMultiSelect=['Golden Four']),dict(startLocation='Firefleas Top')]:
 r=generate(dict(seed=15092133,skill='casual',noAdvancedTechs=True,options=options));assert not r['ok'],r
 results.append(dict(rejected=options,error=r['error']))
(root/'start-seeds/random-options.json').write_text(json.dumps(results,indent=2))
