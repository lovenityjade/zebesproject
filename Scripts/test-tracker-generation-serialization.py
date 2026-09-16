#!/usr/bin/env python3
"""The same embedded runtime must safely serve a generator and a live query."""
import ctypes as C,concurrent.futures,json,os,sys,time
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').exists()
l=C.CDLL(str(root/'Native/build/libsm_native.so'))
for name in ('sm_randomizer_generate','sm_tracker_evaluate'):getattr(l,name).argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
m=json.loads((root/'results/seed-14092026.json').read_text())['manifest']
req=dict(seed=m['seed'],skill=m['skill'],progression=m['progression'],patches=m['patches'])
q=dict(randomized=False,inventory=dict(items=0,beams=0,health=99,reserve=0,missiles=0,supers=0,powerBombs=0,bosses=[0]*8,doors=[0]*64,collected=[0]*100))
def call(name,request):
 buf=C.create_string_buffer(2097152);n=getattr(l,name)(str(root/'Randomizer').encode(),json.dumps(request).encode(),buf,len(buf));assert 0<n<=len(buf)
 result=json.loads(buf.value);assert result['ok'],result;return result
start=time.monotonic()
with concurrent.futures.ThreadPoolExecutor(max_workers=2) as pool:
 generated=pool.submit(call,'sm_randomizer_generate',req)
 tracked=pool.submit(call,'sm_tracker_evaluate',q)
 a,b=generated.result(),tracked.result()
assert a['manifest']['sha256']==m['sha256'],'Live service changed deterministic generation'
assert a['manifest']['solverVerification']['allItemsReachable']
assert [c['name'] for c in b['checks'] if c['state']==1]==['Morphing Ball']
(root/'tracker-results/serialization-verification.json').write_text(json.dumps(dict(passed=True,host=os.uname().nodename,seed=m['seed'],fingerprint=m['sha256'],seconds=time.monotonic()-start,sameSeedAfterSharedRuntime=True,vanillaRulesIsolated=True),indent=2)+'\n')
print('GENERATION + TRACKER SERIALIZATION PASS',time.monotonic()-start,flush=True)
