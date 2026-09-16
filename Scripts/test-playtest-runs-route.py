#!/usr/bin/env python3
"""Isolated gaming-pc regression; never use player save files."""
import sys,uuid
from pathlib import Path
root=Path(sys.argv[1])
s=(root/'test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
s=s.replace("out=root/'tracker-results'","out=root/'playtest/SMTests'")
s=s.replace("out.mkdir(exist_ok=True)","out.mkdir(exist_ok=True,parents=True)")
s=s.replace('ram=l.sm_simulation_ram();l.sm_set_widescreen(1)','ram=l.sm_simulation_ram();l.sm_set_widescreen(1);setup(save)')
s=s.replace('put(0x79f,0);put(0x78b,0);put(0xd914,5);put(0x998,6)', '''
 if fresh:
  put(0xd914,0);put(0x954,0);put(0xde2,4);step()
 put(0x79f,0);put(0x78b,0);put(0xd914,5);put(0x998,6)''')
exec(compile(s,'fixture','exec'))
l.sm_run_has_completion.argtypes=[C.c_char_p,C.c_int]
l.sm_run_import_ngplus.argtypes=[C.c_char_p,C.c_int,C.c_int]
l.sm_run_time.restype=C.c_uint64
l.sm_route_pixels.restype=C.c_void_p
fresh=False;category=0;source=None;save_path=None;serial=uuid.uuid4().hex

def setup(save):
 global save_path
 save_path=save
 for i in range(3):
  assert l.sm_run_configure(i,category)
  if source:assert l.sm_run_import_ngplus(str(source).encode(),1,i)

boot('source-'+serial,False)
assert not l.sm_run_state(1)
assert l.sm_test_all_equipment()
put(0x9c2,1349);put(0x9c6,123);put(0x9ce,27)
expected=read(0x9a2,8)+read(0x9c2,22)
assert l.sm_test_playtest(0,0)==1
source=save_path;source_hash=hashlib.sha256(source.read_bytes()).hexdigest()
assert l.sm_run_has_completion(str(source).encode(),1)
l.sm_shutdown()
fresh=True;category=1
boot('ngplus-'+serial,False)
assert l.sm_run_state(1)==1 and l.sm_run_state(0)==1
assert read(0x9a2,8)+read(0x9c2,22)==expected
assert l.sm_run_state(2)==1
assert l.sm_test_playtest(2,10)==5
assert [l.sm_test_playtest(2,1) for _ in range(2)]==[0,1]
assert l.sm_test_playtest(2,1)==0
assert l.sm_test_playtest(7,0)==1
assert l.sm_test_playtest(2,1)==0, 'A respawn inherited fractional damage'
assert l.sm_test_playtest(3,80)==120
l.sm_set_assisted_walljump(1);l.sm_set_assisted_spacejump(1)
assert l.sm_assisted_walljump()==0 and l.sm_assisted_spacejump()==0
f=l.sm_run_time(0);wait(61);assert l.sm_run_time(0)>=f+60
capture('speedrun-clock');assert l.sm_test_playtest(1,0)==1
ngsave=save_path
assert l.sm_test_playtest(0,0)==1
record=json.loads(next(ngsave.parent.glob('*.run-*.json')).read_text())
assert record['mode']=='ngplus' and record['category']=='noqol' and record['eligible']
assert record['igt_frames']==l.sm_run_time(0)
# A genuine fresh Start (as opposed to Load) must create another run even if
# an earlier attempt already started. NG+ equipment must be reapplied as well.
assert l.sm_test_playtest(10,0)==1
assert not l.sm_run_state(3) and l.sm_run_state(2) and l.sm_run_time(0)==0
put(0x9c2,1);step()
assert read(0x9a2,8)+read(0x9c2,22)==expected
assert l.sm_test_playtest(0,0)==1
records=[json.loads(p.read_text()) for p in ngsave.parent.glob('*.run-*.json')]
assert len(records)==2 and len({r['run_id'] for r in records})==2
assert hashlib.sha256(source.read_bytes()).hexdigest()==source_hash
l.sm_shutdown();print('NGPLUS_GEAR_DAMAGE_CATEGORIES_TIMER_SOURCE_PRESERVED_PASS',flush=True)
# Loading a finished run must preserve NG+ and never apply a fresh equipment refill.
fresh=False;category=0;source=None
exec(s.replace('shutil.copy2(root/\'slot-results/bank.sram.dat\',save)','None').replace(';setup(save)',''))
boot('ngplus-'+serial,False)
assert l.sm_run_state(1) and l.sm_run_state(3)
assert read(0x9a2,8)+read(0x9c2,22)==expected
l.sm_shutdown();print('NGPLUS_RELOAD_PASS',flush=True)
# Record a route with reversals. Replay is read-only and uses original ROM tiles.
exec(s.replace(';setup(save)',''))
l.sm_route_pixels.restype=C.c_void_p
boot('route-'+serial,True)
for b,n in [(128,80),(64,90),(128,80)]:
 for _ in range(n):step(b)
count=l.sm_route_state(0);assert count>4,count
before=read(0,131072)
for at in [0,count//2,count-1,0,count-1]:
 p=l.sm_route_pixels(at);assert p
assert before==read(0,131072)
Image.frombytes('RGBA',(528,320),C.string_at(l.sm_route_pixels(count-1),528*320*4),'raw','BGRA').resize((1056,640),Image.Resampling.NEAREST).save(out/'route.png')
assert l.sm_test_playtest(0,0)==1 and l.sm_route_state(1)
assert l.sm_save();l.sm_shutdown()
print('ROUTE_RECORD_REVISIT_ROM_MAP_READONLY_REPLAY_PASS',count,flush=True)
