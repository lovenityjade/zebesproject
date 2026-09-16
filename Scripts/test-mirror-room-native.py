#!/usr/bin/env python3
"""Controlled native room-load probe; does not claim Mirror playability."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
fixture=(root/'test-native-tweaks.py').read_text().split('for slot,names in enumerate')[0]
fixture=fixture.replace('world-data/libsm_native.so','mirror/libsm_native.so').replace("root/'tweaks/native'","root/'mirror/native'")
exec(compile(fixture,'mirror-room-fixture','exec'))
l.sm_mirror_configure.argtypes=[C.c_int,C.c_int,C.c_char_p]
digest=json.loads((root/'mirror/scrolls.json').read_text())['sha256'].encode()
assert l.sm_start_configure(0,0,None,0,catalog['sha256'].encode())
assert l.sm_mirror_configure(0,1,digest) and activate(0,1) and select(items,100,m['sha256'].encode())
assert l.sm_test_seed_room(0)
for frame in range(1200):
 step()
 if l.sm_state()==8:break
else:raise AssertionError('Mirror room load timeout')
wait(120)
assert word(0x79b)==0x9e9f
capture('mirror-morph-room')
result=dict(room=word(0x79b),state=l.sm_state(),transitionFrames=frame+1,settledFrames=120,samusX=word(0xaf6),samusY=word(0xafa),cpu=l.sm_cpu_opcodes(),nativeSha256=hashlib.sha256(libpath.read_bytes()).hexdigest(),scope='One controlled native room load, not complete Mirror gameplay or Unreal render validation')
assert result['cpu']==0
(root/'mirror/room-results.json').write_text(json.dumps(result,indent=2)+'\n')
l.sm_shutdown();print('MIRROR_ROOM_LOAD_PASS',result,flush=True)
