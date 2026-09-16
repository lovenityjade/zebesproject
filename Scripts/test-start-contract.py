#!/usr/bin/env python3
"""Validate native start/catalog boundaries and backend rejection, gaming-pc only."""
import ctypes as C,json,os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
l=C.CDLL(str(root/'world-data/libsm_native.so'));l.sm_world_data_catalog_supported.argtypes=[C.c_char_p]
l.sm_start_configure.argtypes=[C.c_int,C.c_int,C.POINTER(C.c_uint8),C.c_int,C.c_char_p]
catalog=json.loads((root/'Randomizer/native_world_data.json').read_text());digest=catalog['sha256'].encode()
assert l.sm_world_data_catalog_supported(digest) and not l.sm_world_data_catalog_supported(b'0'*64) and not l.sm_world_data_catalog_supported(None)
historical=[]
for path in sorted((root/'Randomizer/world_catalog_history').glob('*.json')):
 old=json.loads(path.read_text())
 assert l.sm_world_data_catalog_supported(old['sha256'].encode())
 assert l.sm_start_configure(0,0,None,0,old['sha256'].encode())
 historical.append(old['sha256'])
assert l.sm_start_configure(0,0,None,0,digest)
for slot,spawn,patches,count,sha in [(-1,0,None,0,digest),(4,0,None,0,digest),(0,65535,None,0,digest),(0,6,None,0,digest),(0,0,None,0,b'0'*64),(0,0,(C.c_uint8*2)(0,0),2,digest),(0,0,(C.c_uint8*1)(63),1,digest)]:
 assert not l.sm_start_configure(slot,spawn,patches,count,sha)
 assert l.sm_start_spawn(0)==0
l.sm_randomizer_generate.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
errors=[]
for selected in [dict(layoutCustom=['fake_patch']),dict(layoutCustom=['high_jump','high_jump']),dict(doorsColorsRando='on'),dict(startLocation='Golden Four')]:
 b=C.create_string_buffer(2097152);q=dict(seed=15092190,options=selected)
 n=l.sm_randomizer_generate(str(root/'Randomizer').encode(),json.dumps(q).encode(),b,len(b));assert 0<n<=len(b)
 r=json.loads(b.value);assert not r['ok'],r;errors.append(dict(options=selected,error=r['error']))
(root/'start-seeds/contract-verification.json').write_text(json.dumps(dict(nativeCases=7,catalog=digest.decode(),historicalCatalogs=historical,rejections=errors),indent=2));print('START_CONTRACT_PASS',flush=True)
