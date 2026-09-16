#!/usr/bin/env python3
"""Real title/menu navigation and protected UI composition on gaming-pc."""
from pathlib import Path
import shutil
# Reuse the managed A/B/C fixture, with a separate SRAM/output directory.
fixture=Path(__file__).with_name('test-native-slots.py').read_text().split('boot();choose(0)')[0]
fixture=fixture.replace("out=root/'slot-results'", "out=root/'title-menu-results'")
exec(compile(fixture,'slot-fixture','exec'))
assert os.uname().nodename=='gaming-pc'
from PIL import Image
for n in ('sm_wide_scene','sm_wide_overlay','sm_cinema_lights'):getattr(lib,n).restype=C.c_void_p
shutil.copy2(root/'slot-results/bank.sram.dat',sram)
slots[:]=[None,0,1];modes[:]=[0,1,1]
assert lib.sm_init(str(rom).encode(),str(sram).encode());lib.sm_slots_enable(action,None)
for i in range(3):register(i)
lib.sm_set_widescreen(1)
samples=[]
def shot(name):
 native=C.string_at(lib.sm_pixels(),256*240*4)
 scene=C.string_at(lib.sm_wide_scene(),400*240*4);ui=C.string_at(lib.sm_wide_overlay(),400*240*4)
 for y in range(224):
  for x in range(256):
   n=(y*256+x)*4;w=(y*400+x+72)*4
   assert (ui[w:w+3] if ui[w+3] else scene[w:w+3])==native[n:n+3],(name,x,y)
 im=Image.frombytes('RGBA',(400,240),scene,'raw','BGRA');im.putalpha(255)
 im.alpha_composite(Image.frombytes('RGBA',(400,240),ui,'raw','BGRA'))
 im.crop((0,0,400,224)).resize((1200,672),Image.Resampling.NEAREST).save(out/(name+'.png'))
 (out/(name+'-ui.bgra')).write_bytes(ui)
 lights=list((C.c_float*(32*3*4)).from_address(lib.sm_cinema_lights()))
 samples.append(dict(name=name,frame=lib.sm_frame(),state=lib.sm_state(),sub=lib.sm_cinema_state(3),phase=lib.sm_cinema_state(0),sources=lib.sm_cinema_state(1),guiPixels=sum(bool(ui[i]) for i in range(3,len(ui),4)),lights=lights))
 print('PASS',name,lib.sm_state(),lib.sm_cinema_state(1),flush=True)
for i in range(1,3000):
 step()
 if i in (200,500,1000,1500):shot('opening-'+str(i))
 if lib.sm_state()==1 and lib.sm_cinema_state(2)==0x9f29:break
else:raise AssertionError('Title did not become ready')
shot('title');assert samples[-1]['guiPixels']>100 and samples[-1]['sources']>=4
press(8);until(lambda:lib.sm_state()==4 and word(0x727)==4);wait();shot('saves')
while word(0x952)!=3:press(32)
press(8);until(lambda:word(0x727)==8);wait();shot('copy');press(1);until(lambda:word(0x727)==4)
while word(0x952)!=4:press(32)
press(8);until(lambda:word(0x727)==22);wait();shot('clear');press(1);until(lambda:word(0x727)==4)
choose(0);shot('vanilla-options');assert not gray(23) and gray(18)
row(1);press(128);wait();shot('story-preview');assert lib.sm_generation_menu(0)==1 and not lib.sm_generation_menu(3)
row(0);press(8);assert options();row(1);press(128);wait();shot('boss-easy');assert lib.sm_generation_menu(0)==2
row(3)
for level in range(1,4):press(128);wait();shot('boss-'+['easy','medium','hard','hardcore'][level]);assert lib.sm_generation_menu(1)==level
row(0);press(8);assert options()
row(1);press(128);wait();shot('randomized-pending');assert gray(23) and not gray(18)
row(4);press(8);assert lib.sm_generation_take_request();wait();shot('generating')
lib.sm_generation_fail(0);wait();shot('generation-error')
press(8);assert lib.sm_generation_take_request()
items,h=data(0);assert lib.sm_generation_commit(items,100,h,str(sram).encode());slots[0]=0
wait();shot('generated');assert not gray(23) and gray(18)
assert lib.sm_cpu_opcodes()==0
assert all(s['guiPixels']>100 and s['sources']>0 for s in samples if s['state'] in (2,4))
lib.sm_shutdown()
(out/'verification.json').write_text(json.dumps(dict(passed=True,host=os.uname().nodename,compositionByteExact=True,cpuOpcodes=0,nativeSha256=hashlib.sha256((root/'Native/build/libsm_native.so').read_bytes()).hexdigest(),samples=samples),indent=2)+'\n')
print('TITLE/MENUS NATIVE PASS',flush=True)
