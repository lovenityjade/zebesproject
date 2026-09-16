#!/usr/bin/env python3
import ctypes as C,json,tempfile
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parent.parent
lib=C.CDLL(str(ROOT/'Native/build/libsm_native.so'));lib.sm_init.argtypes=[C.c_char_p,C.c_char_p];lib.sm_error.restype=C.c_char_p
for n in ('sm_wide_scene','sm_wide_overlay','sm_scene'):getattr(lib,n).restype=C.c_void_p
with tempfile.TemporaryDirectory(prefix='sm-ceres-wide-') as tmp:
 assert lib.sm_init(bytes(ROOT/'roms/Super Metroid (Japan, USA) (En,Ja).sfc'),bytes(Path(tmp)/'private.sram'))
 lib.sm_set_widescreen(1)
 for _ in range(10000):
  f,s=lib.sm_frame(),lib.sm_state();assert lib.sm_step(8 if f>180 and f%120<2 and not 7<=s<=18 else 0),lib.sm_error()
  if lib.sm_state()==8:break
 for _ in range(90):assert lib.sm_step(0),lib.sm_error()
 assert lib.sm_room()==0xdf45 and lib.sm_ppu_mode()==7 and lib.sm_wide_available()
 wide=C.string_at(lib.sm_wide_scene(),400*240*4);scene=C.string_at(lib.sm_scene(),256*240*4)
 assert all(wide[(y*400+72)*4:(y*400+328)*4]==scene[y*1024:(y+1)*1024] for y in range(32,224))
 out=ROOT/'Docs/Widescreen';out.mkdir(exist_ok=True)
 Image.frombytes('RGBA',(400,240),wide,'raw','BGRA').crop((0,0,400,224)).save(out/'df45-world.png')
 assert lib.sm_cpu_opcodes()==0
 (out/'ceres-verification.json').write_text(json.dumps(dict(passed=True,room='df45',mode=7,width=400,centerByteExact=True,cpuOpcodes=0),indent=2)+'\n')
 lib.sm_shutdown()
print('SM_CERES_WIDE_PASS')
