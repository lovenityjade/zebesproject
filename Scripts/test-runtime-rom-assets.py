#!/usr/bin/env python3
"""Check actual reconstructed native buffers; requires an isolated Linux build."""
import ctypes as C
import hashlib
import json
from pathlib import Path
import subprocess
import sys
root=Path(sys.argv[1]).resolve()
assert (root/'ISOLATED_TEST_DIRECTORY').is_file()
path=root/'Native/build/libsm_native.so'
lib=C.CDLL(str(path));lib.sm_init.argtypes=[C.c_char_p,C.c_char_p];lib.sm_error.restype=C.c_char_p
symbols={p[-1]:int(p[0],16) for line in subprocess.check_output(['nm','-an',str(path)],text=True).splitlines() if len(p:=line.split())==3 and all(c in '0123456789abcdefABCDEF' for c in p[0])}
base=C.cast(lib.sm_init,C.c_void_p).value-symbols['sm_init']
proofs={domain:json.loads((root/f'Docs/Releases/ALPHA-0.24/{domain}-rom-reconstruction.json').read_text()) for domain in ['world','areas','escape','animals']}
names=dict(world='world_patch_bytes',areas='area_data',escape='escape_room_data',animals='animals_bytes')
for domain,proof in proofs.items():assert not any(C.string_at(base+symbols[names[domain]],proof['reconstructedBytes'])),domain
assert not any(C.string_at(base+symbols['tracker_icons'],26*1024))
rom=root/'roms/Super Metroid (Japan, USA) (En,Ja).sfc';before=hashlib.sha256(rom.read_bytes()).hexdigest()
assert lib.sm_init(str(rom).encode(),str(root/'SMTests/runtime-rom-assets.sram').encode()),lib.sm_error()
result=dict(emptyBeforeRom=True,byteExactDomains={})
for domain,proof in proofs.items():
 digest=hashlib.sha256(C.string_at(base+symbols[names[domain]],proof['reconstructedBytes'])).hexdigest()
 assert digest==proof['payloadSha256'],(domain,digest)
 result['byteExactDomains'][domain]=digest
icon=C.CFUNCTYPE(C.c_void_p,C.c_int)(base+symbols['sm_tracker_icon_pixels'])
for index in range(26):
 data=C.string_at(icon(index),1024)
 assert any(data[3::4]),index
 assert set(data[3::4])<={0,255},index
assert not icon(-1) and not icon(26)
result['trackerIcons']=26
result['trackerRgbaSha256']=hashlib.sha256(C.string_at(base+symbols['tracker_icons'],26*1024)).hexdigest()
lib.sm_window_icon_pixels.restype=C.c_void_p
helmet=C.string_at(lib.sm_window_icon_pixels(),1024)
assert any(helmet[3::4]) and set(helmet[3::4])<={0,255}
result['helmetRgbaSha256']=hashlib.sha256(helmet).hexdigest()
lib.sm_shutdown();assert hashlib.sha256(rom.read_bytes()).hexdigest()==before
result.update(sourceRomUnchanged=True,librarySha256=hashlib.sha256(path.read_bytes()).hexdigest())
(root/'SMTests/runtime-rom-assets-verification.json').write_text(json.dumps(result,indent=2)+'\n')
print('RUNTIME_ROM_ASSETS PASS',json.dumps(result))
