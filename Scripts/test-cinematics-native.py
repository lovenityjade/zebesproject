#!/usr/bin/env python3
"""Isolated native cinematic traversal, metadata and original image parity."""
from pathlib import Path
exec(compile(Path(__file__).with_name('test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0],'fixture','exec'))
out=root/'cinematic-results';out.mkdir(exist_ok=True)
(out/'native-verification.json').unlink(missing_ok=True)
l.sm_cinema_lights.restype=C.c_void_p
samples=[]
for case,frames in [(1,[30,45,90,180,400,650]),(2,[120,320,550,800,1050,1250,1800,1900,1950,2000,2050,2200,2600,3000]),(3,[120,240,400,800,1400,2300]),(0,[120,400,900])]:
 boot('SMTests-cinematic-'+str(case),False)
 assert l.sm_test_credits_ending() if case==3 else l.sm_test_cinematic(case)
 for i in range(1,max(frames)+1):
  step()
  if i in frames:
   name=f'case-{case}-{i}'
   image=Image.frombytes('RGBA',(400,240),C.string_at(l.sm_wide_scene(),400*240*4),'raw','BGRA');image.putalpha(255)
   image.alpha_composite(Image.frombytes('RGBA',(400,240),C.string_at(l.sm_wide_overlay(),400*240*4),'raw','BGRA'))
   image.crop((0,0,400,224)).resize((1200,672),Image.Resampling.NEAREST).save(out/(name+'.png'))
   values=list((C.c_float*(32*3*4)).from_address(l.sm_cinema_lights()))
   samples.append(dict(name=name,state=l.sm_state(),scene=l.sm_cinema_state(0),function=hex(l.sm_cinema_state(2)),count=l.sm_cinema_state(1),ship=[word(0xf7a),word(0xf7e),word(0xfb2),word(0x911),word(0x915)],lights=values))
   (out/(name+'-ui.bgra')).write_bytes(C.string_at(l.sm_wide_overlay(),400*240*4))
   # Scene + protected captions exactly reproduce the original reference pixels.
   original=C.string_at(l.sm_pixels(),256*240*4);scene=C.string_at(l.sm_wide_scene(),400*240*4);ui=C.string_at(l.sm_wide_overlay(),400*240*4)
   if not l.sm_credits_state(0) and l.sm_presentation_kind()==2:
    for y in range(224):
     for x in range(256):
      n=(y*256+x)*4;w=(y*400+x+72)*4
      assert (ui[w:w+3] if ui[w+3] else scene[w:w+3])==original[n:n+3],(name,x,y)
 assert l.sm_cpu_opcodes()==0;l.sm_shutdown()
(out/'native-verification.json').write_text(json.dumps(dict(passed=True,compositionByteExact=True,nativeCpuOpcodes=0,nativeSha256=hashlib.sha256((root/'Native/build/libsm_native.so').read_bytes()).hexdigest(),samples=samples),indent=2)+'\n')
print('CINEMATIC NATIVE PASS',flush=True)
