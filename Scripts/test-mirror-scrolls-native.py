#!/usr/bin/env python3
"""Actual Mirror door-scroll dispatch and seed transaction boundaries."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
(root/'mirror').mkdir(exist_ok=True)
fixture=(root/'test-native-tweaks.py').read_text().split('for slot,names in enumerate')[0]
fixture=fixture.replace('world-data/libsm_native.so','mirror/libsm_native.so').replace("root/'tweaks/native'","root/'mirror/native'")
exec(compile(fixture,'mirror-scroll-fixture','exec'))
mirror_audit=json.loads((root/'mirror/scrolls.json').read_text());digest=mirror_audit['sha256'].encode()
l.sm_mirror_configure.argtypes=[C.c_int,C.c_int,C.c_char_p]
enabled=routine('sm_mirror_active',C.c_int)
dispatch=routine('CallDoorDefSetupCode',None,C.c_uint32)
direct=routine('sm_mirror_door',C.c_int,C.c_uint32)
assert not l.sm_mirror_configure(-1,1,digest) and not l.sm_mirror_configure(4,1,digest)
assert not l.sm_mirror_configure(0,2,digest) and not l.sm_mirror_configure(0,1,b'bad')
assert l.sm_mirror_configure(0,1,digest) and l.sm_mirror_configure(1,0,None) and l.sm_mirror_configure(2,1,digest)
assert not enabled() # configuration alone cannot change the running world
results=[]
for slot in [0,1,2,0]:
 assert activate(slot,1);assert select(items,100,m['sha256'].encode())
 assert bool(enabled())==(slot!=1)
 if slot==1:
  C.memset(ram+0xcd20,0xa5,50);dispatch(0x8fb981)
  assert read(0xcd20,50)==bytes([2 if i==6 else 0xa5 for i in range(50)])
  continue
 for p in mirror_audit['programs']:
  C.memset(ram+0xcd20,0xa5,50);before=read(0,131072);expected=bytearray(before)
  for index,value in p['writes']:expected[0xcd20+index]=value
  dispatch(p['address']);assert read(0,131072)==expected,hex(p['address'])
  results.append(dict(slot=slot,address=p['address']))
 assert not direct(0x8f0000)
# A pending different slot and a rejected item plan preserve applied callbacks.
assert activate(1,1) and enabled()
bad=(Item*100)(*items);bad[0].address=0
assert not select(bad,100,m['sha256'].encode()) and enabled()
assert select(items,100,m['sha256'].encode()) and not enabled()
assert activate(0,1) and select(items,100,m['sha256'].encode()) and enabled()
assert l.sm_mirror_configure(0,0,None) and enabled()
assert select(items,100,m['sha256'].encode()) and not enabled()
assert l.sm_mirror_configure(0,1,digest)
assert select(None,0,None) and not enabled()
C.memset(ram+0xcd20,0xa5,50);dispatch(0x8fb981);assert read(0xcd26,1)==b'\x02'
assert l.sm_cpu_opcodes()==0;l.sm_shutdown()
(root/'mirror/scroll-results.json').write_text(json.dumps(dict(cases=results,sourcePrograms=75,pendingCommitAndVanilla=True,cpu=0,nativeSha256=hashlib.sha256(libpath.read_bytes()).hexdigest(),scope='Source scroll routines only; full mirrored geometry, AI and public generation remain unimplemented'),indent=2)+'\n')
print('MIRROR_NATIVE_SCROLL_PASS',len(results),flush=True)
