#!/usr/bin/env python3
"""gaming-pc only: reversible native data transactions, not gameplay parity."""
import ctypes as C,json,os,sys,subprocess,hashlib,uuid
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
out=root/'world-data';out.mkdir(exist_ok=True)
libpath=root/'world-data/libsm_native.so';l=C.CDLL(str(libpath))
class Item(C.Structure):_fields_=[('address',C.c_uint32),('plm',C.c_uint16),('kind',C.c_uint16)]
l.sm_init.argtypes=[C.c_char_p,C.c_char_p];l.sm_seed_stage.argtypes=[C.POINTER(Item),C.c_int,C.c_char_p];l.sm_error.restype=C.c_char_p
l.sm_world_data_capabilities.restype=l.sm_world_data_selected.restype=C.c_uint64;l.sm_world_data_configure.argtypes=[C.c_uint64]
symbols={p[-1]:int(p[0],16) for line in subprocess.check_output(['nm','-an',str(libpath)],text=True).splitlines() if len(p:=line.split())==3 and all(c in '0123456789abcdefABCDEF' for c in p[0])}
base=C.cast(l.sm_init,C.c_void_p).value-symbols['sm_init'];select=C.CFUNCTYPE(C.c_int,C.POINTER(Item),C.c_int,C.c_char_p)(base+symbols['sm_seed_select_plan'])
rom=next((root/'roms').glob('*.sfc'));original=rom.read_bytes();assert len(original)==0x300000
m=json.loads((root/'elevators-hud/seed-0.json').read_text())['manifest'];items=(Item*100)(*[Item(p['address'],p['plm'],p['kind']) for p in m['placements']]);fingerprint=m['sha256'].encode()
assert l.sm_seed_stage(items,100,fingerprint)
folder=out/m['sha256'];folder.mkdir(exist_ok=True)
assert l.sm_init(str(rom).encode(),str(folder/(uuid.uuid4().hex+'.sram')).encode()),l.sm_error()
romptr=C.c_void_p.from_address(base+symbols['g_rom']).value
# The decompiled core installs its own pre-existing runtime ROM fixes at init.
# Compare against that initialized Vanilla image, not raw disk bytes.
assert select(None,0,None)
baseline=C.string_at(romptr,len(original))
catalog=json.loads((root/'Randomizer/native_world_data.json').read_text());audit=json.loads((root/'world-data/dependencies.json').read_text())
l.sm_world_data_catalog_sha256.restype=C.c_char_p
assert l.sm_world_data_catalog_sha256().decode()==catalog['sha256']
mask=(1<<len(catalog['patches']))-1;assert l.sm_world_data_capabilities()==mask
assert l.sm_world_data_configure(mask);assert not l.sm_world_data_configure(1<<63);assert l.sm_world_data_selected()==mask
results=[]
for selected in [0,*[1<<p['id'] for p in catalog['patches']],mask,0]:
 expected=bytearray(baseline)
 for patch in catalog['patches']:
  if selected&(1<<patch['id']):
   for span in ([] if patch.get('nativeBehavior') else audit['patches'][patch['name']]['spans']):
    address=span['address'];values=bytes(span['data']);expected[address:address+len(values)]=values
 for p in m['placements']:expected[p['address']:p['address']+2]=p['plm'].to_bytes(2,'little')
 assert l.sm_world_data_configure(selected);assert select(items,100,fingerprint)
 actual=C.string_at(romptr,len(original));assert actual==expected,(selected,[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:20])
 # Rejected item plans leave the already applied world and item table unchanged.
 wrong=(Item*100)(*items);wrong[0].address=0;assert not select(wrong,100,fingerprint);assert C.string_at(romptr,len(original))==actual
 assert select(None,0,None);assert C.string_at(romptr,len(original))==baseline,('vanilla restore',selected)
 assert select(items,100,fingerprint);assert C.string_at(romptr,len(original))==expected
 results.append(dict(mask=selected,sha256=hashlib.sha256(actual).hexdigest()))
# Overlapping patches must follow VARIA's requested sequence, not bit order.
reverse=list(reversed(range(len(catalog['patches']))));sequence=(C.c_uint8*len(reverse))(*reverse)
l.sm_world_data_configure_order.argtypes=[C.POINTER(C.c_uint8),C.c_int]
l.sm_world_data_get_order.argtypes=[C.POINTER(C.c_uint8),C.c_int]
assert l.sm_world_data_configure_order(sequence,len(reverse))
expected=bytearray(baseline)
for index in reverse:
 for span in ([] if catalog['patches'][index].get('nativeBehavior') else audit['patches'][catalog['patches'][index]['name']]['spans']):
  a=span['address'];expected[a:a+span['size']]=bytes(span['data'])
for p in m['placements']:expected[p['address']:p['address']+2]=p['plm'].to_bytes(2,'little')
assert select(items,100,fingerprint);assert C.string_at(romptr,len(original))==expected
bad=(C.c_uint8*2)(0,0);assert not l.sm_world_data_configure_order(bad,2)
readback=(C.c_uint8*64)();assert l.sm_world_data_get_order(readback,64)==len(reverse) and list(readback)[:len(reverse)]==reverse
results.append(dict(order=reverse,sha256=hashlib.sha256(expected).hexdigest(),duplicateRejected=True))
assert select(None,0,None);assert C.string_at(romptr,len(original))==baseline
assert l.sm_cpu_opcodes()==0;l.sm_shutdown();assert hashlib.sha256(rom.read_bytes()).digest()==hashlib.sha256(original).digest()
(out/'verification.json').write_text(json.dumps(dict(cases=results,patches=len(catalog['patches']),sourceRomUnchanged=True,emulatedCpu=0,scope='Full-ROM byte comparisons and rollback; no room traversal or decompression claims'),indent=2));print('WORLD_DATA_TRANSACTIONS_PASS',len(results),flush=True)
