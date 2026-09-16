#!/usr/bin/env python3
"""Batch presentation decoding must match the exact pixel decoder byte for byte."""
import ctypes as C,json,shutil,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
lib=C.CDLL(str(ROOT/'Native/build/libsm_native.so'));lib.sm_init.argtypes=[C.c_char_p,C.c_char_p];lib.sm_error.restype=C.c_char_p
sizes={'sm_scene':256*240*4,'sm_layers':256*240*4,'sm_wide_scene':400*240*4,'sm_wide_metadata':400*240*4,'sm_wide_background':400*240,'sm_simulation_ram':131072}
for n in sizes:getattr(lib,n).restype=C.c_void_p
reference={};count=0
with tempfile.TemporaryDirectory(prefix='sm-renderer-parity-') as tmp:
 for fast in (0,1):
  save=Path(tmp)/f'{fast}.sram';shutil.copy2(ROOT/'Unreal/Saved/SMPreview/sram.dat',save)
  assert lib.sm_init(bytes(ROOT/'roms/Super Metroid (Japan, USA) (En,Ja).sfc'),bytes(save)),lib.sm_error()
  lib.sm_set_fast_scene_render(fast);lib.sm_set_widescreen(1);lib.sm_set_engine_weather(1);lib.sm_set_parallax(0,0,0);lib.sm_set_assisted_walljump(0)
  for _ in range(8500):
   f,s=lib.sm_frame(),lib.sm_state();assert lib.sm_step(8 if f>180 and f%120<2 and not 7<=s<=18 else 0),lib.sm_error()
   if lib.sm_state()==8:break
  for dest in range(8):
   assert lib.sm_teleport(dest)
   for _ in range(240):assert lib.sm_step(0),lib.sm_error()
   for phase in range(4):
    lib.sm_set_parallax((phase-2)*3,phase,phase%2)
    for _ in range(3):assert lib.sm_step(0),lib.sm_error()
    for n,size in sizes.items():
     key=(dest,phase,n);data=C.string_at(getattr(lib,n)(),size)
     if not fast:reference[key]=data
     elif data!=reference[key]:
      (ROOT/'.tmp/render-fast.bin').write_bytes(data);(ROOT/'.tmp/render-reference.bin').write_bytes(reference[key])
      raise AssertionError((key,sum(a!=b for a,b in zip(data,reference[key]))))
     else:count+=1
   print('PASS renderer',fast,dest,hex(lib.sm_room()),flush=True)
  assert lib.sm_cpu_opcodes()==0;lib.sm_shutdown()
lib.sm_set_fast_scene_render(1)
out=ROOT/'Docs/Widescreen';out.mkdir(exist_ok=True)
(out/'renderer-parity.json').write_text(json.dumps(dict(passed=True,rooms=8,parallaxCases=4,buffersCompared=count,byteExact=True,nativeRamIncluded=True,cpuOpcodes=0),indent=2)+'\n')
print('SM_RENDERER_PARITY_PASS')
