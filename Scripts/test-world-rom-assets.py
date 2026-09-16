#!/usr/bin/env python3
"""Compare the actual native ROM reconstruction against a private pinned IPS fixture.
Requires world-dependencies-private.json in an isolated Linux build tree.
That fixture contains source data and must never be staged or published.
"""
from pathlib import Path
import ctypes as C,hashlib,json,subprocess,sys
root=Path(sys.argv[1]).resolve();assert (root/'ISOLATED_TEST_DIRECTORY').is_file()
path=root/'Native/build/libsm_native.so';lib=C.CDLL(str(path))
symbols={p[-1]:int(p[0],16) for line in subprocess.check_output(['nm','-an',str(path)],text=True).splitlines() if len(p:=line.split())==3 and all(c in '0123456789abcdefABCDEF' for c in p[0])}
base=C.cast(lib.sm_init,C.c_void_p).value-symbols['sm_init']
def routine(name,result,*args):return C.CFUNCTYPE(result,*args)(base+symbols[name])
class Item(C.Structure):_fields_=[('address',C.c_uint32),('plm',C.c_uint16),('kind',C.c_uint16)]
lib.sm_seed_stage.argtypes=[C.POINTER(Item),C.c_int,C.c_char_p]
lib.sm_init.argtypes=[C.c_char_p,C.c_char_p];lib.sm_error.restype=C.c_char_p
lib.sm_world_data_configure.argtypes=[C.c_uint64]
lib.sm_mirror_configure.argtypes=[C.c_int,C.c_int,C.c_char_p]
seed=json.loads((root/'SMTests/appimage-seed.json').read_text())['manifest']
items=(Item*100)(*[Item(p['address'],p['plm'],p.get('kind',0)) for p in seed['placements']])
assert lib.sm_seed_stage(items,100,seed['sha256'].encode())
rom=root/'roms/Super Metroid (Japan, USA) (En,Ja).sfc';original=rom.read_bytes()
save=root/'SMTests'/seed['sha256']/'world-reconstruction.sram';save.parent.mkdir(exist_ok=True)
assert lib.sm_init(str(rom).encode(),str(save).encode()),lib.sm_error()
proof=json.loads((root/'Docs/Releases/ALPHA-0.24/world-rom-reconstruction.json').read_text())
payload=C.string_at(base+symbols['world_patch_bytes'],proof['reconstructedBytes'])
assert hashlib.sha256(payload).hexdigest()==proof['payloadSha256']
select=routine('sm_seed_select_plan',C.c_int,C.POINTER(Item),C.c_int,C.c_char_p)
assert select(None,0,None)
ptr=C.c_void_p.from_address(base+symbols['g_rom']).value
baseline=C.string_at(ptr,len(original))
catalog=json.loads((root/'Randomizer/native_world_data.json').read_text())
audit=json.loads((root/'world-dependencies-private.json').read_text());cases=[]
for selection in [0,*[1<<p['id'] for p in catalog['patches']],(1<<len(catalog['patches']))-1,0]:
 expected=bytearray(baseline)
 for patch in catalog['patches']:
  if selection&(1<<patch['id']):
   for span in ([] if patch.get('nativeBehavior') else audit['patches'][patch['name']]['spans']):
    a=span['address'];expected[a:a+len(span['data'])]=bytes(span['data'])
 for p in seed['placements']:expected[p['address']:p['address']+2]=p['plm'].to_bytes(2,'little')
 # Independent full-map correction, applied by all randomized plans.
 expected[0x1a820e:0x1a8210]=b'\x25\xcc'
 assert lib.sm_world_data_configure(selection)
 assert select(items,100,seed['sha256'].encode())
 current=C.string_at(ptr,len(original))
 assert current==expected,(selection,[(hex(i),a,b) for i,(a,b) in enumerate(zip(current,expected)) if a!=b][:20])
 assert select(None,0,None) and C.string_at(ptr,len(original))==baseline
 cases.append(selection)
for slot in range(4):
 assert lib.sm_mirror_configure(slot,0,None)
 assert not lib.sm_mirror_configure(slot,1,None)
assert not lib.sm_mirror_configure(-1,0,None)
lib.sm_shutdown()
assert rom.read_bytes()==original
result=dict(byteExactNativePayload=True,transactionCases=len(cases),all46PatchIds=True,vanillaRollback=True,excludedMirrorRejected=True,sourceRomUnchanged=True,librarySha256=hashlib.sha256(path.read_bytes()).hexdigest())
(root/'SMTests/world-rom-native-verification.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
