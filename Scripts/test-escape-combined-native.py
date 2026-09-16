#!/usr/bin/env python3
"""Validate the public Escape + Fast Tourian + Scavenger native composition."""
import os,sys,textwrap
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
combined_source=(root/'test-escape-objectives-native.py').read_text()
exec(compile(combined_source.split('for slot in range(3):')[0],'escape-combined-fixture','exec'))
slot=0
body=combined_source.split('for slot in range(3):\n',1)[1].split(' # Native dependency revalidation',1)[0]
body=body.replace('escape/integration/seed-{slot:02d}','escape/public/seed-03')
body=body.replace(" op=plan(",""" hunt=manifest['nativeContext']['scavenger']
 class Hunt(C.Structure):
  _fields_=[('version',C.c_uint32),('size',C.c_uint32),('count',C.c_uint16),('order',C.c_uint16*17)]
 h=Hunt();h.version=1;h.size=C.sizeof(h);h.count=len(hunt['words']);h.order[:h.count]=hunt['words']
 l.sm_scavenger_configure.argtypes=[C.c_int,C.POINTER(Hunt),C.c_char_p]
 assert l.sm_scavenger_configure(slot,C.byref(h),hunt['catalogSha256'].encode())
 op=plan(""")
# The Scavenger quota is deliberately still unmet after only killing bosses.
body=body.replace(' assert state(4) and bool(event(14))==(slot!=0)',' assert not state(4) and not event(14)')
exec(compile(textwrap.dedent(body),'escape-fast-scavenger-plan','exec'))
assert routine('sm_tourian_fast',C.c_int)() and enabled()
assert l.sm_scavenger_state(0)==h.count and l.sm_scavenger_state(1)==0
assert l.sm_cpu_opcodes()==0
(root/'escape/combined-native-results.json').write_text(json.dumps(dict(passed=True,seed=manifest['seed'],fingerprint=manifest['sha256'],fastTourian=True,escape=True,scavengerTargets=h.count,quotaRespectsUncollectedHunt=True,mapTotals=obj['mapTotals'],nativeSha256=hashlib.sha256((root/'escape/libsm_native.so').read_bytes()).hexdigest()),indent=2)+'\n')
l.sm_shutdown();print('ESCAPE_COMBINED_NATIVE_PASS',flush=True)
