#!/usr/bin/env python3
"""Run the original introductory story, preserving text and a constant canvas."""
import ctypes as C,json,tempfile
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parent.parent
out=ROOT/'Docs/DisplayFixes';out.mkdir(exist_ok=True)
lib=C.CDLL(str(ROOT/'Native/build/libsm_native.so'))
lib.sm_init.argtypes=[C.c_char_p,C.c_char_p];lib.sm_error.restype=C.c_char_p
for n in ('sm_pixels','sm_scene','sm_ui_overlay','sm_wide_scene','sm_wide_overlay'):getattr(lib,n).restype=C.c_void_p
samples=[];story=False
with tempfile.TemporaryDirectory(prefix='SMTests-story-') as tmp:
 assert lib.sm_init(bytes(ROOT/'roms/Super Metroid (Japan, USA) (En,Ja).sfc'),bytes(Path(tmp)/'fresh.sram'))
 lib.sm_set_widescreen(1)
 for _ in range(20000):
  f,s=lib.sm_frame(),lib.sm_state()
  buttons=8 if not story and f>180 and f%120<2 and not 7<=s<=18 else 0
  if story and f>2400 and f%900<2:buttons=2 # Advance the original story pages.
  assert lib.sm_step(buttons),lib.sm_error()
  if lib.sm_state()==30:story=True
  assert lib.sm_wide_available(),'Canvas unavailable during boot/story/fade'
  f=lib.sm_frame()
  if story and f in (1200,1800,2400,4000,6000,8000):
   assert lib.sm_presentation_kind()==2,(f,lib.sm_state())
   native=C.string_at(lib.sm_pixels(),256*240*4)
   scene=C.string_at(lib.sm_wide_scene(),400*240*4);ui=C.string_at(lib.sm_wide_overlay(),400*240*4)
   count=0
   for y in range(224):
    for x in range(256):
     i=(y*400+x+72)*4;j=(y*256+x)*4
     composed=ui[i:i+3] if ui[i+3] else scene[i:i+3]
     assert composed==native[j:j+3],('Text/scene composition changed native content',f,x,y)
     count+=bool(ui[i+3])
   image=Image.frombytes('RGBA',(400,240),scene,'raw','BGRA');image.alpha_composite(Image.frombytes('RGBA',(400,240),ui,'raw','BGRA'))
   image.crop((0,0,400,224)).save(out/f'story-{f}-native.png')
   samples.append(dict(frame=f,state=lib.sm_state(),width=400,protectedTextPixels=count,compositionByteExact=True))
   print('PASS story',f,count,flush=True)
  if story and lib.sm_state()==8:break
 assert story and lib.sm_state()==8 and lib.sm_room()==0xdf45,(lib.sm_state(),hex(lib.sm_room()))
 assert len(samples)>=3 and any(s['protectedTextPixels'] for s in samples)
 assert lib.sm_cpu_opcodes()==0
 lib.sm_shutdown()
(out/'story-verification.json').write_text(json.dumps(dict(passed=True,samples=samples,reachedCeres=True,cpuOpcodes=0,note='Original cinematic content stays centered at 1:1 in the 400px canvas; no invented side scenery.'),indent=2)+'\n')
print('SM_STORY_PRESENTATION_PASS')
