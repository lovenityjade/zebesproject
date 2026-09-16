#!/usr/bin/env python3
"""Exercise the embedded read-only validator; run only in the isolated test root."""
import ctypes as C
import json
import os
from pathlib import Path
import sys
import time

root = Path(sys.argv[1]).resolve()
assert os.uname().nodename == 'gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').exists()
lib = C.CDLL(str(root/'Native/build/libsm_native.so'))
lib.sm_randomizer_validate.argtypes = [C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]

def validate(request):
    before = json.dumps(request,sort_keys=True)
    buffer = C.create_string_buffer(262144)
    started = time.monotonic()
    count = lib.sm_randomizer_validate(str(root/'Randomizer').encode(),before.encode(),buffer,len(buffer))
    assert 0 < count <= len(buffer)
    result = json.loads(buffer.value)
    assert json.dumps(request,sort_keys=True) == before
    assert 'manifest' not in result
    print(round(time.monotonic()-started,3),result,flush=True)
    return result

assert validate(dict(seed=0))['ok']
assert not validate(dict(seed=-1))['ok']
q = dict(seed=0,options=dict(maxDifficulty='easy'),skillSettings={'Ridley':"I'm scared!"})
r = validate(q)
assert not r['ok'] and any('Ridley' in i['message'] and i['page']==8 and i['otherPage']==1 for i in r['issues'])
q['options']['maxDifficulty'] = 'hard'
assert validate(q)['ok']
r = validate(dict(relicHunt=dict(enabled=True),options=dict(tourian='Fast',escapeRando='on',majorsSplit='Scavenger')))
assert not r['ok'] and len(r['issues']) == 3
assert not validate(dict(relicHunt=dict(enabled=True,placed=5,required=6)))['ok']
assert not validate(dict(options=dict(objective=['kill all G4','kill kraid'])))['ok']
assert validate(dict(options=dict(objective=['kill kraid','kill phantoon'])))['ok']
assert not validate(dict(patches=['unknown']))['ok']
assert validate(dict(seed=0,options=dict(maxDifficulty='random')))['ok']
# Each call owns a fresh interpreter: the previous boss/goal settings cannot leak.
assert validate(dict(seed=0))['ok']
print('SETTINGS PREFLIGHT PASS',flush=True)
