#!/usr/bin/env python3
"""Full HUD, item-dialog lifecycle and real door transitions, with private saves."""
import ctypes as C, hashlib, json, shutil, tempfile, time
from pathlib import Path
from PIL import Image
ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'Docs/DisplayFixes';OUT.mkdir(exist_ok=True)
lib=C.CDLL(str(ROOT/'Native/build/libsm_native.so'))
lib.sm_init.argtypes=[C.c_char_p,C.c_char_p];lib.sm_error.restype=C.c_char_p
for n in ('sm_pixels','sm_scene','sm_wide_scene','sm_wide_overlay','sm_ui_overlay','sm_simulation_ram'):
 getattr(lib,n).restype=C.c_void_p
source=ROOT/'Unreal/Saved/SMPreview/sram.dat'
user=ROOT/'Unreal/Saved/SM/sram.dat';before=hashlib.sha256(user.read_bytes()).hexdigest()
rom=ROOT/'roms/Super Metroid (Japan, USA) (En,Ja).sfc'
checks=[]
def step(buttons=0):
 assert lib.sm_step(buttons),lib.sm_error()
def read(name,w):return C.string_at(getattr(lib,name)(),w*240*4)
def img(name,w):return Image.frombytes('RGBA',(w,240),read(name,w),'raw','BGRA').crop((0,0,w,224))
def shot(name):
 img('sm_pixels',256).save(OUT/f'{name}-native.png')
 scene=img('sm_wide_scene',400);scene.alpha_composite(img('sm_wide_overlay',400));scene.save(OUT/f'{name}-wide.png')
def put(address,value):C.c_uint16.from_address(lib.sm_simulation_ram()+address).value=value

def boot(save,wide):
 shutil.copy2(source,save)
 assert lib.sm_init(bytes(rom),bytes(save)),lib.sm_error()
 lib.sm_set_widescreen(wide);lib.sm_set_engine_weather(1);lib.sm_set_parallax(0,0,0);lib.sm_set_assisted_walljump(0)
 for _ in range(8500):
  f,s=lib.sm_frame(),lib.sm_state();step(8 if f>180 and f%120<2 and not 7<=s<=18 else 0)
  if lib.sm_state()==8:break
 assert lib.sm_teleport(1)
 for _ in range(440):step()
 assert lib.sm_test_all_equipment()
 for _ in range(20):step()

with tempfile.TemporaryDirectory(prefix='SMTests-display-') as tmp:
 save=Path(tmp)/'full.sram';boot(save,1)
 # Copy a real checksummed native save, never the user's SRAM.
 fixture=ROOT/'Unreal/Saved/SMTests/HUD-all-items.sram';fixture.parent.mkdir(exist_ok=True)
 shutil.copy2(save,fixture)
 # All five weapon groups plus energy and AUTO reserves. Verify both flashing
 # phases, full counters, low counters and empty reserves against native pixels.
 hud_cases=0
 for health,missiles,super_,bombs,reserve in [(1499,230,50,50,400),(99,5,1,0,0),(100,100,10,10,1)]:
  for addr,value in [(0x9c2,health),(0x9c6,missiles),(0x9ca,super_),(0x9ce,bombs),(0x9d6,reserve)]:put(addr,value)
  for selected in range(6):
   put(0x9d2,selected)
   for phase in range(2):
    for _ in range(8):step()
    native=read('sm_pixels',256);hud=read('sm_wide_overlay',400)
    for y in range(31):
     for start,end,dest in [(0,80,0),(80,208,152)]:
      for x in range(start,end):
       expected=native[(y*256+x)*4:(y*256+x)*4+3]
       p=(y*400+dest+x-start)*4
       assert hud[p:p+3]==expected,(health,selected,phase,y,x,'HUD pixel lost')
       assert hud[p+3]==(255 if any(expected) else 0)
    assert not any(hud[(31*400)*4:(31*400+336)*4]),'Old separator'
    for y in range(24):
     assert all(native[((y+7)*256+208)*4:((y+7)*256+248)*4][c::4]==hud[(y*400+352)*4:(y*400+392)*4][c::4] for c in range(3)), 'Minimap differs from native overlap'
    hud_cases+=1
   if health==1499:shot(f'hud-weapon-{selected}')
 assert lib.sm_cpu_opcodes()==0
 lib.sm_shutdown()
 # Compare actual door and message lifecycles with wide off/on. The native
 # message intentionally waits 360 frames for its item fanfare before input.
 timelines={};performance={}
 for wide in (0,1):
  boot(Path(tmp)/f'transition-{wide}.sram',wide)
  timeline=[];durations=[];messages=0;overlay_seen=False
  put(0xdc8,9) # Native queued item-message path, including HDMA open/close.
  for t in range(430):
   started=time.perf_counter();step(2 if t>=385 else 0);durations.append((time.perf_counter()-started)*1000)
   active=bool(lib.sm_message_active());messages+=active
   timeline.append((lib.sm_state(),lib.sm_room(),lib.sm_samus_x(),lib.sm_samus_y(),active))
   if wide:
    assert lib.sm_wide_available(),'Canvas disappeared during item message'
    ui=read('sm_ui_overlay',256);native=read('sm_pixels',256);hud=read('sm_wide_overlay',400)
    for y in range(32,224):
     a=ui[y*1024:(y+1)*1024];b=hud[(y*400+72)*4:(y*400+328)*4]
     assert a==b,'Dialog moved or clipped'
     for x in range(256):
      i=y*1024+x*4
      if ui[i+3]:
       assert ui[i:i+3]==native[i:i+3],'Dialog text altered'
       overlay_seen=True
    if t in (5,30,200,405):shot(f'message-{t}')
  assert messages>350 and not lib.sm_message_active(),('Message did not close',wide,messages)
  if wide:assert overlay_seen,'Message overlay absent'
  transitions=[]
  for t in range(750):
   started=time.perf_counter();step(128 | (512 if t%20<10 else 0) | (256 if 70<=t<100 else 0));durations.append((time.perf_counter()-started)*1000)
   timeline.append((lib.sm_state(),lib.sm_room(),lib.sm_samus_x(),lib.sm_samus_y(),bool(lib.sm_message_active())))
   if lib.sm_state() in (9,10,11):transitions.append(t)
   if wide:
    assert lib.sm_wide_available(),'Canvas disappeared during door'
    if t in (40,65,85,105,130,160,200,300,400,500):shot(f'door-{t}')
  assert transitions and lib.sm_state()==8 and lib.sm_room()!=0x93d5,('Door did not finish',wide,lib.sm_state(),hex(lib.sm_room()))
  paused=False;equipment=False
  for t in range(460):
   step(8 if t<8 or 320<=t<328 else (2048 if 120<=t<128 else 0))
   timeline.append((lib.sm_state(),lib.sm_room(),lib.sm_samus_x(),lib.sm_samus_y(),False))
   paused|=lib.sm_state()==15
   equipment|=lib.sm_state()==15 and C.c_uint16.from_address(lib.sm_simulation_ram()+0x727).value==1
   if wide and t in (90,230,420):
    shot(f'pause-{t}')
    assert lib.sm_wide_available()
    native=read('sm_pixels',256);hud=read('sm_wide_overlay',400)
    for y in range(31):
     for a,b,d in [(0,80,0),(80,208,152)]:
      n=native[(y*256+a)*4:(y*256+b)*4];h=hud[(y*400+d)*4:(y*400+d+b-a)*4]
      assert all(n[c::4]==h[c::4] for c in range(3)),('Pause HUD moved or clipped',t,y)
  assert paused and equipment and lib.sm_state()==8,('Pause lifecycle incomplete',wide,paused,equipment,lib.sm_state())
  assert lib.sm_test_escape_timer();timer_pixels=0
  for t in range(340):
   step()
   timeline.append(C.string_at(lib.sm_simulation_ram()+0x943,9))
   ui=read('sm_ui_overlay',256);native=read('sm_pixels',256)
   for i in range(32*1024,224*1024,4):
    if ui[i+3]:
     assert ui[i:i+3]==native[i:i+3],('Countdown pixels changed',wide,t)
     timer_pixels+=1
   if wide and t in (30,300):
    shot(f'timer-{t}')
    hud=read('sm_wide_overlay',400)
    for y in range(32,224):assert ui[y*1024:(y+1)*1024]==hud[(y*400+72)*4:(y*400+328)*4]
  assert timer_pixels>1000,('Countdown UI absent',wide,timer_pixels)
  timelines[wide]=timeline
  performance[wide]=dict(transitionFrames=len(transitions),messageFrames=messages,maxStepMs=max(durations),meanStepMs=sum(durations)/len(durations))
  assert lib.sm_cpu_opcodes()==0;lib.sm_shutdown()
 assert timelines[0]==timelines[1],'Widescreen changed native transition timing or movement'
assert hashlib.sha256(user.read_bytes()).hexdigest()==before
report=dict(passed=True,hudCases=hud_cases,allWeaponsAndReservesPixelExact=True,messagePixelsExact=True,
            pauseMapAndEquipmentHudExact=True,countdownPixelsExact=True,dialogAndDoorTimingUnchanged=True,timings=performance,userSaveUnchanged=True,
            testSave=str(fixture.relative_to(ROOT)),scope='Real native door and queued message lifecycles; full HUD pixel correspondence. GPU captures checked separately.')
(OUT/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('SM_DISPLAY_REGRESSIONS_PASS',json.dumps(report),flush=True)
