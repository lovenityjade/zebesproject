#!/usr/bin/env python3
"""Headless original-map fixtures. Requires the isolated gaming-pc test root."""
import ctypes as C, hashlib, json, os, shutil, sys
from pathlib import Path
from PIL import Image
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
os.chdir(root);out=root/'full-map-results';out.mkdir(exist_ok=True)
rom=next((root/'roms').glob('*.sfc'));source=rom.read_bytes();rom_hash=hashlib.sha256(source).hexdigest()
class Item(C.Structure):_fields_=[('address',C.c_uint32),('plm',C.c_uint16)]
def pc(a):return ((a>>16)&127)*32768+(a&32767)
def maps(area):
 ptr=int.from_bytes(source[pc(0x82964a)+area*3:pc(0x82964a)+area*3+3],'little')
 tiles=[int.from_bytes(source[pc(ptr)+i*2:pc(ptr)+i*2+2],'little') for i in range(2048)]
 ptr=0x820000|int.from_bytes(source[pc(0x829717)+area*2:pc(0x829717)+area*2+2],'little')
 return tiles,source[pc(ptr):pc(ptr)+256]
def setup(path):
 l=C.CDLL(str(path));l.sm_init.argtypes=[C.c_char_p,C.c_char_p];l.sm_error.restype=C.c_char_p
 l.sm_seed_stage.argtypes=[C.POINTER(Item),C.c_int,C.c_char_p]
 for n in ('sm_simulation_ram','sm_pixels','sm_pause_map','sm_wide_scene','sm_wide_overlay','sm_vram'):getattr(l,n).restype=C.c_void_p
 return l
old=setup(root/'Native/build/libsm_native-before-full-map.so');new=setup(root/'Native/build/libsm_native.so')
def run(lib,area,seed=None,label=''):
 assert lib.sm_seed_clear()
 manifest=None
 if seed:
  manifest=json.loads((root/f'results/seed-{seed}.json').read_text())['manifest']
  items=(Item*100)(*[Item(p['address'],p['plm']) for p in manifest['placements']])
  assert lib.sm_seed_stage(items,100,manifest['sha256'].encode())
 directory=out/(label+'-'+str(area))/(manifest['sha256'] if manifest else 'vanilla');directory.mkdir(parents=True,exist_ok=True)
 save=directory/'sram.dat';shutil.copy2(root/'slot-results/bank.sram.dat',save)
 assert lib.sm_init(str(rom).encode(),str(save).encode()),lib.sm_error()
 lib.sm_set_widescreen(0)
 ram=lib.sm_simulation_ram()
 def read(o,n):return C.string_at(ram+o,n)
 def word(o):return int.from_bytes(read(o,2),'little')
 def setword(o,v):C.memmove(ram+o,int(v).to_bytes(2,'little'),2)
 def step(b=0):assert lib.sm_step(b),lib.sm_error()
 def press(b):step(b);step()
 for i in range(9000):
  step(8 if i>180 and i%120<2 else 0)
  if lib.sm_state()==4 and word(0x727)==4:break
 else:raise AssertionError('No file menu')
 for _ in range(3):step()
 for _ in range(8):
  if word(0x952)==1:break
  press(32)
 else:raise AssertionError(('Cannot select B',lib.sm_state(),word(0x727),word(0x952)))
 press(8)
 for _ in range(1000):
  step()
  if lib.sm_state()==2 and word(0xde2)==3:break
 else:raise AssertionError('No options')
 # Enter the native Landing Site loader directly; intro/menu timing is outside
 # this map fixture. Both comparison builds use the same loaded slot state.
 setword(0x79f,0);setword(0x78b,0);setword(0xd914,5);setword(0x998,6)
 for _ in range(2000):
  step()
  if lib.sm_state()==8:break
 else:raise AssertionError('No gameplay')
 print('GAMEPLAY',label,area,lib.sm_state(),word(0x998),hex(lib.sm_room()),word(0x952),flush=True)
 for _ in range(440):step()
 lib.sm_set_widescreen(1)
 # Controlled map-area fixture, not a claim of travelling to all these rooms.
 # Clear discovery so secret visibility cannot be explained by prior exploration.
 setword(0x79f,area);setword(0x789,0)
 C.memset(ram+0x7f7,0,256);C.memset(ram+0xcd52,0,2048);C.memset(ram+0xd908,0,8)
 for _ in range(3):step()
 hud=read(0xc608,192)
 tiles,known=maps(area)
 mx=word(0x7a1)+(word(0xaf6)>>8);my=word(0x7a3)+(word(0xafa)>>8)+1
 for y in range(3):
  for x in range(5):
   gx=mx+x-2;gy=my+y-1;i=(gx&31)+gy*32+(gx&32)*32
   explored_here=read(0x7f7+i//8,1)[0]&(0x80>>(i&7))
   expected_tile=tiles[i]&1023 if seed or explored_here else 31
   got=word(0xc608+2*(26+y*32+x))&1023
   assert got==expected_tile,('Native minimap',area,x,y,got,expected_tile)
 before=read(0x7f7,256)+read(0xcd52,2048)+read(0xd908,8)
 press(8)
 # Original Ceres forbids pressing Start to pause. Exercise its authored map
 # renderer explicitly without enabling a new Ceres gameplay/pause feature.
 if area==6:setword(0x998,12)
 for _ in range(300):
  step()
  if lib.sm_state()==15:break
 assert lib.sm_state()==15,(area,lib.sm_state())
 for _ in range(80):step()
 tiles,known=maps(area);vram=C.string_at(lib.sm_vram()+0x6000,4096);actual=[int.from_bytes(vram[2*i:2*i+2],'little') for i in range(2048)]
 explored=read(0x7f7,256)
 expected=[t&~0x400 if explored[i>>3]&(0x80>>(i&7)) else (t if seed and t&1023!=31 else 31) for i,t in enumerate(tiles)]
 if actual!=expected:
  print('BAD MAP',label,lib.sm_state(),word(0x998),word(0x763),word(0x727),hex(word(0x4000)),flush=True)
  Image.frombytes('RGBA',(256,240),C.string_at(lib.sm_pixels(),256*240*4),'raw','BGRA').save(out/'failure.png')
 assert actual==expected,(label,area,next((i,hex(actual[i]),hex(expected[i])) for i in range(2048) if actual[i]!=expected[i]))
 # Pause stores the genuinely explored current area, but does not fake discovery.
 assert sum(b.bit_count() for b in explored)<16
 assert read(0xd908,8)==bytes(8), 'Faked station activation'
 assert sum(b.bit_count() for b in read(0xcd52,2048))<16
 hidden=[i for i,t in enumerate(tiles) if t&1023!=31 and not known[i>>3]&(0x80>>(i&7))]
 if seed:
  assert all(actual[i]&1023==tiles[i]&1023 for i in hidden)
  if area<6:assert hidden,(area,'No secret-map fixture')
 native=C.string_at(lib.sm_pixels(),256*240*4)
 full=C.string_at(lib.sm_pause_map(),512*256*4)
 if seed==14092026 and area in (0,1,4):
  Image.frombytes('RGBA',(512,256),full,'raw','BGRA').save(out/f'{area}-full.png')
  scene=Image.frombytes('RGBA',(400,240),C.string_at(lib.sm_wide_scene(),400*240*4),'raw','BGRA')
  scene.alpha_composite(Image.frombytes('RGBA',(400,240),C.string_at(lib.sm_wide_overlay(),400*240*4),'raw','BGRA'))
  scene.save(out/f'{area}-pause.png')
 digest=hashlib.sha256(read(0,131072)+native+full+hud).hexdigest()
 bounds=[word(o) for o in (0x5ac,0x5ae,0x5b0,0x5b2)]
 if seed:
  occupied=[(i%32+(i//1024)*32,(i%1024)//32) for i,t in enumerate(tiles) if t&1023!=31]
  expected_bounds=[min(x for x,y in occupied)*8-(24 if area==4 else 0),max(x for x,y in occupied)*8,min(y for x,y in occupied)*8,max(y for x,y in occupied)*8]
  assert bounds==expected_bounds,('Secret map scroll bounds',area,bounds,expected_bounds)
 lib.sm_shutdown();assert lib.sm_seed_clear()
 return dict(area=area,authoredTiles=sum(t&1023!=31 for t in tiles),secretTiles=len(hidden),digest=digest,bounds=bounds)
report=[]
for area in range(7):
 a=run(old,area,label='before');b=run(new,area,label='vanilla')
 assert a==b,('Vanilla changed',area,a,b)
 for seed in (14092026,14092027):
  r=run(new,area,seed,'random-'+str(seed));r['seed']=seed;report.append(r)
 print('PASS area',area,flush=True)
assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(out/'verification.json').write_text(json.dumps(dict(passed=True,host=os.uname().nodename,cases=report,vanillaExactParityAreas=7,sourceRomUnchanged=True,nativeSha256=hashlib.sha256((root/'Native/build/libsm_native.so').read_bytes()).hexdigest(),scope='Controlled map-area fixtures through native pause and minimap; two seeds, no fake exploration flags.'),indent=2)+'\n')
print('ALL FULL MAP CHECKS PASSED',flush=True)
