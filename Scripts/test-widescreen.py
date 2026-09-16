#!/usr/bin/env python3
import ctypes as C,hashlib,json,shutil,tempfile
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parent.parent
lib=C.CDLL(str(ROOT/'Native/build/libsm_native.so'))
lib.sm_init.argtypes=[C.c_char_p,C.c_char_p];lib.sm_error.restype=C.c_char_p
for n in ('sm_wide_scene','sm_wide_overlay','sm_scene','sm_pixels'):getattr(lib,n).restype=C.c_void_p
source=ROOT/'Unreal/Saved/SMPreview/sram.dat';before=source.read_bytes();out=ROOT/'Docs/Widescreen';out.mkdir(exist_ok=True)
rows=[]
with tempfile.TemporaryDirectory(prefix='sm-wide-') as tmp:
 save=Path(tmp)/'private.sram';shutil.copy2(source,save)
 assert lib.sm_init(bytes(ROOT/'roms/Super Metroid (Japan, USA) (En,Ja).sfc'),bytes(save)),lib.sm_error()
 for _ in range(8500):
  f,s=lib.sm_frame(),lib.sm_state();assert lib.sm_step(8 if f>180 and f%120<2 and not 7<=s<=18 else 0),lib.sm_error()
  if lib.sm_state()==8:break
 lib.sm_set_widescreen(1);lib.sm_set_engine_weather(1)
 for dest in range(8):
  assert lib.sm_teleport(dest)
  for _ in range(300):assert lib.sm_step(0),lib.sm_error()
  assert lib.sm_wide_available() and lib.sm_state()==8
  wide=C.string_at(lib.sm_wide_scene(),400*240*4);scene=C.string_at(lib.sm_scene(),256*240*4)
  hud=C.string_at(lib.sm_wide_overlay(),400*240*4)
  assert all(wide[(y*400+72)*4:(y*400+328)*4]==scene[y*1024:(y+1)*1024] for y in range(32,224)),('Center mismatch',dest)
  assert not any(hud[(32*400)*4:]),'HUD outside top 32 rows'
  assert all(hud[(31*400+x)*4+3]==0 for x in range(336)), 'Old separator present'
  room=f'{lib.sm_room():04x}'
  Image.frombytes('RGBA',(400,240),wide,'raw','BGRA').crop((0,0,400,224)).save(out/f'{room}-world.png')
  Image.frombytes('RGBA',(400,240),hud,'raw','BGRA').crop((0,0,400,224)).save(out/f'{room}-hud.png')
  rows.append(dict(destination=dest,room=room,centerByteExact=True,worldWidth=400,hudRows=32,camera=[lib.sm_camera_x(),lib.sm_camera_y()]))
  print('PASS',dest,room,flush=True)
 assert lib.sm_cpu_opcodes()==0
 lib.sm_shutdown()
assert source.read_bytes()==before
(out/'verification.json').write_text(json.dumps(dict(passed=True,rooms=rows,cpuOpcodes=0,userSaveUnchanged=True,note='Side pixels are decoded from real room data; space beyond a room boundary stays empty.'),indent=2)+'\n')
print('SM_WIDESCREEN_PASS')
