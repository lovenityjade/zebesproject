#!/usr/bin/env python3
"""Verify runtime UI reconstruction against private frozen pixel fixtures.
Run only in an isolated Linux build. Fixture .inc files are not release assets.
"""
from pathlib import Path
import ctypes as C, hashlib, json, re, subprocess, sys
root=Path(sys.argv[1]).resolve()
assert (root/'ISOLATED_TEST_DIRECTORY').is_file()
path=root/'Native/build/libsm_native.so'
lib=C.CDLL(str(path));lib.sm_init.argtypes=[C.c_char_p,C.c_char_p];lib.sm_error.restype=C.c_char_p
symbols={p[-1]:int(p[0],16) for line in subprocess.check_output(['nm','-an',str(path)],text=True).splitlines()
         if len(p:=line.split())==3 and all(c in '0123456789abcdefABCDEF' for c in p[0])}
base=C.cast(lib.sm_init,C.c_void_p).value-symbols['sm_init']
fixtures=root/'SMTests/asset-baseline'
for symbol,size in [('varia_gfx',4096),('objective_pause_gfx',16384)]:
 assert not any(C.string_at(base+symbols[symbol],size)),symbol+' present before ROM load'
rom=root/'roms/Super Metroid (Japan, USA) (En,Ja).sfc';before=hashlib.sha256(rom.read_bytes()).hexdigest()
assert lib.sm_init(str(rom).encode(),str(root/'SMTests/ui-rom-test.sram').encode()),lib.sm_error()
results={}
for file,symbol,stride in [('sm_varia_assets.inc','varia_gfx',16),('sm_objective_pause_assets.inc','objective_pause_gfx',32)]:
 source=(fixtures/file).read_text();literal=re.search(r'\b'+symbol+r'\[[^;]+?=\s*\{(.*?)\};',source,re.S).group(1)
 expected=bytes(int(x,0) for x in re.findall(r'0x[0-9a-fA-F]+|\d+',literal))
 current=C.string_at(base+symbols[symbol],len(expected))
 proof=json.loads((root/'Docs/Releases/ALPHA-0.24'/f'{symbol}-rom-reconstruction.json').read_text())
 assert hashlib.sha256(current).hexdigest()==proof['reconstructedSha256']
 for t in proof['usedTiles']:assert current[t*stride:(t+1)*stride]==expected[t*stride:(t+1)*stride],(symbol,t)
 results[symbol]=dict(usedTiles=len(proof['usedTiles']),allUsedTilesByteExact=True,emptyBeforeRom=True,sha256=hashlib.sha256(current).hexdigest())
lib.sm_shutdown();assert hashlib.sha256(rom.read_bytes()).hexdigest()==before
result=dict(sourceRomUnchanged=True,librarySha256=hashlib.sha256(path.read_bytes()).hexdigest(),sheets=results)
(root/'SMTests/ui-rom-native-verification.json').write_text(json.dumps(result,indent=2)+'\n')
print('UI_ROM_ASSETS PASS',json.dumps(result))
