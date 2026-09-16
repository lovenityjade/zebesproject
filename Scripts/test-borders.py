#!/usr/bin/env python3
"""Native frames and simulation must match with decorative continuation on/off."""
import ctypes as C,json,shutil,tempfile
from pathlib import Path
from PIL import Image
R=Path(__file__).resolve().parent.parent;out=R/'Docs/VisualPause/Borders';out.mkdir(parents=True,exist_ok=True)
l=C.CDLL(str(R/'Native/build/libsm_native.so'));l.sm_init.argtypes=[C.c_char_p,C.c_char_p];l.sm_error.restype=C.c_char_p
for n in ['sm_pixels','sm_wide_scene','sm_wide_overlay','sm_simulation_ram']:getattr(l,n).restype=C.c_void_p
baseline={};rows=[]
def step(b=0):assert l.sm_step(b),l.sm_error()
def get(n,size):return C.string_at(getattr(l,n)(),size)
with tempfile.TemporaryDirectory(prefix='SMTests-borders-') as t:
 for enabled in (0,1):
  save=Path(t)/f'{enabled}.sram';shutil.copy2(R/'Unreal/Saved/SMTests/HUD-all-items.sram',save)
  assert l.sm_init(bytes(R/'roms/Super Metroid (Japan, USA) (En,Ja).sfc'),bytes(save))
  l.sm_set_border_extension(enabled);l.sm_set_widescreen(1);l.sm_set_engine_weather(1)
  for _ in range(8500):
   f,s=l.sm_frame(),l.sm_state();step(8 if f>180 and f%120<2 and not 7<=s<=18 else 0)
   if l.sm_state()==8:break
  for dest in range(8):
   assert l.sm_teleport(dest)
   for _ in range(240):step()
   native=get('sm_pixels',256*240*4);ram=get('sm_simulation_ram',131072)
   key=f'{l.sm_room():04x}'
   if not enabled:baseline[key]=(native,ram)
   else:assert baseline[key]==(native,ram),('Simulation or original frame changed',key)
   image=Image.frombytes('RGBA',(400,240),get('sm_wide_scene',400*240*4),'raw','BGRA')
   image.alpha_composite(Image.frombytes('RGBA',(400,240),get('sm_wide_overlay',400*240*4),'raw','BGRA'))
   image.crop((0,0,400,224)).save(out/f'{key}-{enabled}.png')
   if enabled:rows.append(dict(room=key,samples=l.sm_border_stat(0),unresolved=l.sm_border_stat(1),nativeExact=True))
  assert l.sm_cpu_opcodes()==0;l.sm_shutdown()
assert sum(x['samples'] for x in rows)>0
(out/'verification.json').write_text(json.dumps(dict(passed=True,rooms=rows),indent=2)+'\n')
print('SM_BORDERS_PASS',rows)
