from pathlib import Path
exec(compile(Path(__file__).with_name('test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0],'fixture','exec'))
out=root/'credits-ending-diagnostic';out.mkdir(exist_ok=True)
boot('SMTests-original-ending',False)
assert l.sm_test_credits_ending()
for _ in range(18000):
 step()
 if l.sm_credits_state(0)==1:break
assert l.sm_credits_state(0)==1
# Traverse the custom roll, then inspect the untouched native ending.
for _ in range(20000):
 step()
 if l.sm_credits_state(0)==0:break
else:raise AssertionError('Original ending never completed')
for i in range(1,2201):
 step()
 if i in [700,800,1000,1600,2200]:
  capture('custom-'+str(i));print(i,hex(word(0x1f51)),flush=True)
  Image.frombytes('RGBA',(256,240),C.string_at(l.sm_pixels(),256*240*4),'raw','BGRA').crop((0,0,256,224)).resize((768,672)).save(out/('raw-'+str(i)+'.png'))
  (out/('ram-'+str(i)+'.bin')).write_bytes(read(0,131072))
  (out/('vram-'+str(i)+'.bin')).write_bytes(C.string_at(l.sm_vram(),65536))
l.sm_shutdown()
