#!/usr/bin/env python3
import ctypes as C,json,shutil,tempfile
from pathlib import Path
R=Path(__file__).resolve().parent.parent;l=C.CDLL(str(R/'Native/build/libsm_native.so'))
l.sm_init.argtypes=[C.c_char_p,C.c_char_p];l.sm_error.restype=C.c_char_p;l.sm_simulation_ram.restype=C.c_void_p
with tempfile.TemporaryDirectory(prefix='SMTests-electric-') as d:
 p=Path(d)/'test.sram';shutil.copy2(R/'Unreal/Saved/SMTests/HUD-all-items.sram',p)
 assert l.sm_init(bytes(R/'roms/Super Metroid (Japan, USA) (En,Ja).sfc'),bytes(p))
 def step(b=0):assert l.sm_step(b),l.sm_error()
 def ram(a):return C.c_uint16.from_address(l.sm_simulation_ram()+a).value
 for _ in range(8500):
  f,s=l.sm_frame(),l.sm_state();step(8 if f>180 and f%120<2 and not 7<=s<=18 else 0)
  if l.sm_state()==8:break
 for _ in range(400):step()
 assert l.sm_test_awaken();assert l.sm_test_room(0x9f11,39,139)
 for _ in range(440):step()
 print('enemies',[(hex(ram(0xf78+i*64)),ram(0xf7a+i*64),ram(0xf7e+i*64)) for i in range(16) if ram(0xf78+i*64)],flush=True)
 grapples=screws=deaths=0
 for t in range(1300):
  b=128 if t<8 else 0
  if any(a<=t<a+2 for a in [20,30,40,50]):b=4
  if 70<=t<170:b=512
  if 190<=t<192:b=2
  if 210<=t<240:b=512
  if 240<=t<250:b=128
  if 250<=t<285:b=128|256
  if 340<=t<520:b=64|512
  if t==550:
   assert l.sm_teleport(0)
   for _ in range(440):step()
  if 960<=t<968:b=128
  if any(a<=t<a+2 for a in [1000,1010,1020,1030]):b=4
  if 1050<=t<1130:b=16|(512 if t%16<8 else 0)
  if 1150<=t<1160:b=128
  if 1160<=t<1190:b=128|256
  step(b)
  grapples+=bool(l.sm_visual_state(34,0));screws+=bool(l.sm_visual_state(35,0))
  deaths+=sum(l.sm_visual_state(17,i)==128 for i in range(l.sm_visual_state(14,0)))
  if t in [18,22,32,42,52,80,100,160,240,270]:print(t,'item',ram(0x9d2),'pose',hex(ram(0xa1c)),'grapple',hex(ram(0xd32)),l.sm_visual_state(34,0),flush=True)
 l.sm_shutdown()
report=dict(grappleFrames=grapples,screwFrames=screws,enemyDeaths=deaths,passed=bool(grapples and screws and deaths))
(R/'Docs/VisualPause/electric-core-verification.json').write_text(json.dumps(report,indent=2)+'\n')
print(report)
