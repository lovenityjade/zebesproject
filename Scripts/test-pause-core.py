#!/usr/bin/env python3
"""Validate native inventory actions, original wireframe and explored-only map."""
import ctypes as C,json,shutil,tempfile
from pathlib import Path
from PIL import Image
R=Path(__file__).resolve().parent.parent;out=R/'Docs/VisualPause';out.mkdir(exist_ok=True)
l=C.CDLL(str(R/'Native/build/libsm_native.so'));l.sm_init.argtypes=[C.c_char_p,C.c_char_p];l.sm_error.restype=C.c_char_p
for n in ['sm_pause_map','sm_pause_wire','sm_pause_atlas','sm_pixels','sm_simulation_ram']:getattr(l,n).restype=C.c_void_p
with tempfile.TemporaryDirectory(prefix='SMTests-pause-') as t:
 p=Path(t)/'test.sram';shutil.copy2(R/'Unreal/Saved/SMTests/HUD-all-items.sram',p)
 assert l.sm_init(bytes(R/'roms/Super Metroid (Japan, USA) (En,Ja).sfc'),bytes(p))
 def step(b=0):assert l.sm_step(b),l.sm_error()
 for _ in range(8500):
  f,s=l.sm_frame(),l.sm_state();step(8 if f>180 and f%120<2 and not 7<=s<=18 else 0)
  if l.sm_state()==8:break
 for _ in range(440):step()
 assert not l.sm_pause_action(0,0),'Menu operation allowed during gameplay'
 for i in range(100):step(8 if i<8 else 0)
 assert l.sm_state()==15
 before=C.string_at(l.sm_simulation_ram(),131072)
 # Asset extraction must be read-only, even when repeated.
 for _ in range(3):l.sm_pause_map();l.sm_pause_wire();l.sm_pause_atlas()
 assert before==C.string_at(l.sm_simulation_ram(),131072)
 for i in range(110):step(2048 if i<8 else 0)
 assert l.sm_pause_data(11)==1
 raw=C.string_at(l.sm_pixels(),256*240*4)
 wire=C.string_at(l.sm_pause_wire(),64*136*4)
 # Compare all visible native wireframe pixels with the equipment screen.
 errors=[]
 for y in range(136):
  for x in range(64):
   a=(y*64+x)*4;b=((y+55)*256+x+96)*4
   if wire[a+3] and any(wire[a:a+3]):errors.extend(abs(wire[a+c]-raw[b+c]) for c in range(3))
 # Both use the same native tiles and the PPU brightness conversion table.
 Image.frombytes('RGBA',(256,240),raw,'raw','BGRA').save(out/'pause-native-inspect.png')
 Image.frombytes('RGBA',(64,136),wire,'raw','BGRA').save(out/'wire-inspect.png')
 assert errors and max(errors)==0,('Wireframe differs from native',max(errors))
 Image.frombytes('RGBA',(64,136),wire,'raw','BGRA').save(out/'samus-original.png')
 original=[l.sm_pause_data(i) for i in range(4)]
 for i in range(14):
  assert l.sm_pause_action(0,i)
  assert not (l.sm_pause_data(3)&12==12),'Spazer and Plasma simultaneously enabled'
  assert l.sm_pause_action(0,i)
 # Restore Plasma after the explicit Spazer exclusion check.
 if l.sm_pause_data(3)&4:assert l.sm_pause_action(0,3)
 if not l.sm_pause_data(3)&8:assert l.sm_pause_action(0,4)
 assert [l.sm_pause_data(i) for i in range(4)]==original
 assert l.sm_pause_action(1,0) and l.sm_pause_data(4)==2
 C.c_uint16.from_address(l.sm_simulation_ram()+0x9c2).value=1400
 assert l.sm_pause_action(2,0)
 assert l.sm_pause_data(9)==1499 and l.sm_pause_data(5)==301
 assert l.sm_pause_action(1,0) and l.sm_pause_data(4)==1
 # Reveal no map on a fresh unexplored current-area buffer.
 map_before=C.string_at(l.sm_simulation_ram()+0x7f7,256)
 C.memset(l.sm_simulation_ram()+0x7f7,0,256)
 station=C.c_uint8.from_address(l.sm_simulation_ram()+0xd908+l.sm_area());had_station=station.value;station.value=0
 empty=C.string_at(l.sm_pause_map(),512*256*4)
 assert not any(empty[i] for i in range(len(empty)) if i%4!=3),'Unexplored map revealed'
 C.memmove(l.sm_simulation_ram()+0x7f7,map_before,256);station.value=had_station
 for i in range(120):step(8 if i<8 else 0)
 assert l.sm_state()==8 and l.sm_pause_data(3)==original[3]
 assert l.sm_cpu_opcodes()==0;l.sm_shutdown()
report=dict(passed=True,inventoryActions=28,beamExclusion=True,reserveTransfer=True,wireMaxError=max(errors),readOnlyAssets=True,nativeResume=True)
(out/'pause-core-verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('SM_PAUSE_CORE_PASS',report)
