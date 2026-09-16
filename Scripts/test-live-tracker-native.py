#!/usr/bin/env python3
"""Real C-core frames + embedded oracle + native pause/minimap pixel checks."""
import ctypes as C, hashlib,json,os,shutil,sys
from pathlib import Path
from PIL import Image
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').exists()
assert not (root/'Docs/References/PopTracker/pack').exists()
out=root/'tracker-results';out.mkdir(exist_ok=True)
class Item(C.Structure):_fields_=[('address',C.c_uint32),('plm',C.c_uint16)]
class Snapshot(C.Structure):
 _fields_=[('version',C.c_uint32),('size',C.c_uint32),('session',C.c_uint64),('revision',C.c_uint64),
  ('frame',C.c_uint32),('randomized',C.c_uint32),('world_valid',C.c_uint32)]+[(n,C.c_uint16) for n in ['slot','state','room','area','map_x','map_y','acquired_items','active_items','acquired_beams','active_beams','max_health','max_missiles','max_supers','max_power_bombs','max_reserve']]+[(n,C.c_uint8*s) for n,s in [('collected',100),('item_bits',64),('boss_bits',8),('events',8),('opened_doors',64),('explored_current',256),('explored_saved',2048),('map_stations',8)]]+[('seed_fingerprint',C.c_char*65)]
assert C.sizeof(Snapshot)==2688
l=C.CDLL(str(root/'Native/build/libsm_native.so'));l.sm_init.argtypes=[C.c_char_p,C.c_char_p];l.sm_error.restype=C.c_char_p
l.sm_seed_stage.argtypes=[C.POINTER(Item),C.c_int,C.c_char_p]
l.sm_tracker_snapshot.argtypes=[C.POINTER(Snapshot),C.c_uint32];l.sm_tracker_publish.argtypes=[C.POINTER(Snapshot),C.POINTER(C.c_uint8),C.c_uint32]
l.sm_tracker_evaluate.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int];l.sm_tracker_color.restype=C.c_uint32
for n in ['sm_simulation_ram','sm_wide_scene','sm_wide_overlay','sm_ui_overlay','sm_pixels','sm_vram']:getattr(l,n).restype=C.c_void_p
rom=next((root/'roms').glob('*.sfc'));rom_hash=hashlib.sha256(rom.read_bytes()).hexdigest()
m=json.loads((root/'results/seed-14092026.json').read_text())['manifest'];t=m['tracker']
geometry=sorted(json.loads((root/'Randomizer/native_locations.json').read_text())['locations'],key=lambda c:c['address'])
ram=None

def read(o,n):return C.string_at(ram+o,n)
def word(o):return int.from_bytes(read(o,2),'little')
def put(o,v):C.memmove(ram+o,int(v).to_bytes(2,'little'),2)
def step(b=0):assert l.sm_step(b),l.sm_error()
def press(b):step(b);step()
def wait(n):
 for _ in range(n):step()
def snap():
 s=Snapshot();assert l.sm_tracker_snapshot(C.byref(s),C.sizeof(s));return s
def query(s):
 inv=dict(items=s.acquired_items,beams=s.acquired_beams,health=s.max_health,reserve=s.max_reserve,
  missiles=s.max_missiles,supers=s.max_supers,powerBombs=s.max_power_bombs,bosses=list(s.boss_bits),
  doors=list(s.opened_doors),collected=list(s.collected))
 q=dict(randomized=bool(s.randomized),inventory=inv)
 if s.randomized:q.update(settings=t['settings'],topology=t['topology'])
 buf=C.create_string_buffer(262144);n=l.sm_tracker_evaluate(str(root/'Randomizer').encode(),json.dumps(q).encode(),buf,len(buf));assert 0<n<=len(buf)
 r=json.loads(buf.value);assert r['ok'],r
 states=(C.c_uint8*100)(*[c['state'] for c in r['checks']]);assert l.sm_tracker_publish(C.byref(s),states,100)
 return states

def boot(label,randomized):
 global ram
 assert l.sm_seed_clear()
 if randomized:
  items=(Item*100)(*[Item(i['address'],i['plm']) for i in m['placements']]);assert l.sm_seed_stage(items,100,m['sha256'].encode())
 directory=out/label/(m['sha256'] if randomized else 'vanilla');directory.mkdir(parents=True,exist_ok=True)
 save=directory/'sram.dat';shutil.copy2(root/'slot-results/bank.sram.dat',save)
 assert l.sm_init(str(rom).encode(),str(save).encode());ram=l.sm_simulation_ram();l.sm_set_widescreen(1)
 for i in range(9000):
  step(8 if i>180 and i%120<2 else 0)
  if l.sm_state()==4 and word(0x727)==4:break
 else:raise AssertionError('File menu')
 wait(3)
 for _ in range(8):
  if word(0x952)==1:break
  press(32)
 assert word(0x952)==1;press(8)
 for _ in range(1000):
  step()
  if l.sm_state()==2 and word(0xde2)==3:break
 else:raise AssertionError('Options')
 put(0x79f,0);put(0x78b,0);put(0xd914,5);put(0x998,6)
 for _ in range(2000):
  step()
  if l.sm_state()==8:break
 else:raise AssertionError('Gameplay')
 wait(440)

def capture(label):
 image=Image.frombytes('RGBA',(400,240),C.string_at(l.sm_wide_scene(),400*240*4),'raw','BGRA')
 image.alpha_composite(Image.frombytes('RGBA',(400,240),C.string_at(l.sm_wide_overlay(),400*240*4),'raw','BGRA'))
 image.crop((0,0,400,224)).resize((1200,672),Image.Resampling.NEAREST).save(out/(label+'.png'))
 (out/(label+'.snapshot')).write_bytes(bytes(snap()))

def colors_on_minimap(states,width):
 s=snap();p=l.sm_wide_overlay() if width==400 else l.sm_ui_overlay();ui=C.string_at(p,width*240*4)
 groups={}
 for c,state in zip(geometry,states):
  if c['area']!=s.area:continue
  key=c['mapX'],c['mapY'];groups[key]=groups.get(key,0)|state
 seen=[]
 for (mx,my),state in groups.items():
  x=mx-s.map_x+(4 if width==400 else 2);y=my-s.map_y+1
  if not (0<=x<(7 if width==400 else 5) and 0<=y<(4 if width==400 else 3)):continue
  if (mx,my)==(s.map_x,s.map_y) or state not in (1,2,4,8):continue
  x=(336 if width==400 else 208)+x*8+4;y=y*8+4
  p=ui[(y*width+x)*4:(y*width+x)*4+4];rgb=p[2]<<16|p[1]<<8|p[0]
  assert rgb==l.sm_tracker_color(state),(width,mx,my,state,hex(rgb))
  seen.append([width,mx,my,state])
 return seen

def colors_on_map(states):
 ui=C.string_at(l.sm_wide_overlay(),400*240*4);sx=C.c_int16(word(0xb1)).value;sy=C.c_int16(word(0xb3)).value
 groups={}
 for c,state in zip(geometry,states):
  if c['area']!=word(0x79f):continue
  key=c['mapX'],c['mapY'];groups[key]=groups.get(key,0)|state
 seen=[]
 for (mx,my),state in groups.items():
  x=mx*8+4-sx+72;y=my*8+4-sy
  if x-3<8 or x+3>=392 or y-3<48 or y+3>=192 or state==0:continue
  # A single-state square must use the exact RGB at its center.
  if state not in (1,2,4,8):continue
  p=ui[(y*400+x)*4:(y*400+x)*4+4];rgb=p[2]<<16|p[1]<<8|p[0]
  assert rgb==l.sm_tracker_color(state),(mx,my,x,y,state,hex(rgb))
  seen.append([mx,my,state])
 return seen

palette=[0x3f3f3f,0x20ff20,0xcf1010,0xff9f20,0xffff20,0xafff20,0xef5500,0xff9f20,0x3040ff,0x20ffff,0xc010ff,0xff9f20,0x20d0d0,0xff9f20,0x20d0d0,0xff9f20]
assert [l.sm_tracker_color(i) for i in range(16)]==palette
parity=[];render_cases=[];minimap_cases=[];stale_snapshot=None
for enabled in [False,True]:
 boot('enabled' if enabled else 'disabled',True)
 if stale_snapshot:assert not l.sm_tracker_publish(C.byref(stale_snapshot),old_states,100),'Prior session accepted'
 l.sm_tracker_configure(enabled,1,1)
 # Actual room transition to Morph room, no emulated CPU instructions.
 assert l.sm_test_seed_room(1)
 for _ in range(2000):
  step()
  if l.sm_state()==8 and l.sm_room()==0x9e9f:break
 else:raise AssertionError('Morph room')
 wait(440);s=snap();states=query(s);step();capture(('on' if enabled else 'off')+'-minimap')
 if enabled:
  minimap_cases+=colors_on_minimap(states,400)+colors_on_minimap(states,256)
  assert minimap_cases
 # A change of inventory must reject a worker result from before that change.
 old=word(0x9a4);put(0x9a4,old^4)
 assert not l.sm_tracker_publish(C.byref(s),states,100)
 put(0x9a4,old);assert l.sm_tracker_publish(C.byref(s),states,100)
 press(8)
 for _ in range(1000):
  step()
  if l.sm_state()==15:break
 assert l.sm_state()==15;wait(80)
 capture(('on' if enabled else 'off')+'-pause')
 parity.append(hashlib.sha256(read(0,131072)).hexdigest())
 if enabled:
  render_cases+=colors_on_map(states)
  assert render_cases
  # Equip toggle must not remove the acquired icon or change reachability.
  put(0x9a4,0xffff);put(0x9a8,0x100f)
  for o,v in [(0x9c4,1499),(0x9c8,230),(0x9cc,50),(0x9d0,50),(0x9d4,400)]:put(o,v)
  s=snap();states=query(s);step();capture('all-items-pause');render_cases+=colors_on_map(states)
  old=word(0x9a2);put(0x9a2,0)
  assert l.sm_tracker_publish(C.byref(s),states,100),'Equipped state affected acquired inventory logic'
  put(0x9a2,old)
 stale_snapshot=snap();old_states=states
 assert l.sm_cpu_opcodes()==0;l.sm_shutdown()
assert len(set(parity))==1,'Tracker changed native simulation RAM'
# Vanilla stays original by default; opt-in adds map knowledge without SRAM exploration/seed changes.
boot('vanilla-opt-in',False)
assert not l.sm_seed_active();exploration=read(0x7f7,256);stations=read(0xd908,8)
original_plms=[l.sm_seed_rom_item(i) for i in range(100)]
l.sm_tracker_configure(1,1,1);states=query(snap());press(8)
for _ in range(1000):
 step()
 if l.sm_state()==15:break
wait(80);capture('vanilla-opt-in-pause');assert not l.sm_seed_active()
assert read(0xd908,8)==stations
assert [l.sm_seed_rom_item(i) for i in range(100)]==original_plms
source=rom.read_bytes()
def pc(a):return ((a>>16)&127)*32768+(a&32767)
ptr=int.from_bytes(source[pc(0x82964a):pc(0x82964a)+3],'little')
tiles=[int.from_bytes(source[pc(ptr)+2*i:pc(ptr)+2*i+2],'little') for i in range(2048)]
vram=C.string_at(l.sm_vram()+0x6000,4096)
assert all((int.from_bytes(vram[2*i:2*i+2],'little')&1023)==(tile&1023) for i,tile in enumerate(tiles))
# Toggle from the already-open native pause map, then back on. The actual
# map VRAM must follow immediately; no reset or map-station flag required.
l.sm_tracker_configure(0,1,1)
disabled_vram=C.string_at(l.sm_vram()+0x6000,4096)
assert disabled_vram!=vram
l.sm_tracker_configure(1,1,1)
assert C.string_at(l.sm_vram()+0x6000,4096)==vram
assert read(0xd908,8)==stations
l.sm_shutdown();assert l.sm_seed_clear()
assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
report=dict(passed=True,host=os.uname().nodename,packAbsent=True,simulationRamParity=True,
 palette=palette,visibleChecksVerified=render_cases,minimapChecksVerified=minimap_cases,staleInventoryRejected=True,staleSessionRejected=True,
 vanillaOptInOriginalPlacements=True,vanillaToggleWhilePaused=True,nativeCpuOpcodes=0,sourceRomUnchanged=True,
 nativeSha256=hashlib.sha256((root/'Native/build/libsm_native.so').read_bytes()).hexdigest())
(out/'native-verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('NATIVE TRACKER PASS',len(render_cases),flush=True)
