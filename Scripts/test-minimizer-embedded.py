#!/usr/bin/env python3
"""Production requests through the exact native generation API used by Unreal."""
import ctypes as C,hashlib,json,os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
lib=root/'minimizer/libsm_native.so';native=C.CDLL(str(lib))
generate=native.sm_randomizer_generate
generate.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
reports=[]
for file in sorted((root/'minimizer/public').glob('seed-*.json')):
    expected=json.loads(file.read_text())['manifest']
    request=dict(seed=expected['seed'],skill=expected['skill'],**expected['requestedSettings'])
    buffer=C.create_string_buffer(2097152)
    n=generate(str(root/'Randomizer').encode(),json.dumps(request).encode(),buffer,len(buffer))
    assert 0<n<=len(buffer)
    result=json.loads(buffer.value)
    assert result.get('ok'),result
    actual=result['manifest']
    assert actual['sha256']==expected['sha256'],(actual['seed'],actual['sha256'],expected['sha256'])
    assert actual['nativeContext']==expected['nativeContext']
    assert actual['tracker']==expected['tracker']
    assert actual['solverVerification']['allItemsReachable'] and actual['solverVerification']['completionVerified']
    reports.append(dict(seed=actual['seed'],fingerprint=actual['sha256']))
    print('MINIMIZER_EMBEDDED_PASS',actual['seed'],flush=True)
assert len(reports)==3
(root/'minimizer/public-embedded.json').write_text(json.dumps(dict(passed=True,seeds=reports,
    nativeSha256=hashlib.sha256(lib.read_bytes()).hexdigest(),scope=__doc__),indent=2)+'\n')
