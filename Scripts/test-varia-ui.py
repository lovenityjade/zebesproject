#!/usr/bin/env python3
"""Native VARIA UI/reserve fixtures on gaming-pc, without any interactive launch."""
import sys
from pathlib import Path
# Reuse the isolated core boot, snapshot and rendering fixture helpers only.
fixture=Path(__file__).with_name('test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
exec(compile(fixture,'tracker-fixture','exec'))
out=root/'varia-ui-results';out.mkdir(exist_ok=True)
l.sm_varia_ui_configure.argtypes=[C.c_uint];l.sm_varia_ui_state.argtypes=[C.c_int]
report=dict(sourceRomUnchanged=False)

def equip():
 for o,v in [(0x9a2,0xf32f),(0x9a4,0xf32f),(0x9a6,0x100f),(0x9a8,0x100f),
 (0x9c2,1499),(0x9c4,1499),(0x9c6,120),(0x9c8,230),(0x9ca,8),(0x9cc,50),
 (0x9ce,3),(0x9d0,50),(0x9d4,400),(0x9d6,400),(0x9c0,1)]:put(o,v)
def pause():
 for _ in range(8):step(8)
 for _ in range(1000):
  step()
  if l.sm_state()==15:break
 assert l.sm_state()==15;wait(80)

def resume():
 for _ in range(8):step(8)
 for _ in range(1000):
  step()
  if l.sm_state()==8:break
 assert l.sm_state()==8;wait(80)

def image_bytes():return C.string_at(l.sm_pixels(),256*240*4)
# Vanilla: options enabled/disabled give identical game state and exact pixels.
parity=[]
for flag in (0,15):
 boot('vanilla-'+str(flag),False);l.sm_varia_ui_configure(flag);equip();wait(4)
 assert l.sm_varia_ui_state(0)==0
 parity.append(hashlib.sha256(read(0,131072)+image_bytes()).hexdigest());l.sm_shutdown()
assert parity[0]==parity[1];report['vanillaOptionsExactParity']=True

boot('SMTests-randomized',True);l.sm_varia_ui_configure(15);equip();wait(5)
assert l.sm_varia_ui_state(0)==15
assert l.sm_varia_ui_state(1)==1
expected=sum(c['graphArea']=='Crateria' for c in t['locations'])
assert l.sm_varia_ui_state(2)==expected,(l.sm_varia_ui_state(2),expected)
capture('hud-full-reserves')
# Both presentation widths carry identical original glyph pixels.
wide=C.string_at(l.sm_wide_overlay(),400*240*4)
l.sm_set_widescreen(0);step();narrow=C.string_at(l.sm_ui_overlay(),256*240*4)
for y in range(8,32):
 assert wide[(y*400+152)*4:(y*400+224)*4]==narrow[(y*256+80)*4:(y*256+152)*4]
Image.frombytes('RGBA',(256,240),image_bytes(),'raw','BGRA').crop((0,0,256,224)).resize((768,672),Image.Resampling.NEAREST).save(out/'hud-4x3.png')
l.sm_set_widescreen(1);step()
put(0x9c2,699);step();capture('hud-half-tanks');put(0x9c2,99);step();capture('hud-empty-tanks');put(0x9c2,1499);step()
report['ammoGlyphParityBothWidths']=True
report['areaRemainingCounter']=True
for value,name in [(200,'partial'),(0,'empty')]:
 put(0x9d6,value);step();assert l.sm_varia_ui_state(5)==(1 if value else 0);capture('hud-'+name+'-reserves')
# Remaining counter uses location flags, not equipment ownership.
loc=next(c for c in t['locations'] if c['graphArea']=='Crateria');bit=loc['collectionBit']
item_offset=0xd870
C.memmove(ram+item_offset+bit//8,bytes([read(item_offset+bit//8,1)[0]|(1<<(bit%8))]),1)
step();assert l.sm_varia_ui_state(2)==expected-1
# Three-digit supers and PBs fit the exact 24-pixel columns.
put(0x9ca,105);put(0x9cc,125);put(0x9ce,100);put(0x9d0,150);wait(2);capture('hud-three-digit-ammo')
# Real manual reserve UI, including surplus preservation and cancel.
pause()
for _ in range(8):step(2048)
wait(160)
assert word(0x763)==1
put(0x9c0,2);put(0x9c2,1490);put(0x9d6,200);put(0x755,0x100);put(0x757,0)
press(256);wait(20)
assert word(0x9c2)==1499 and word(0x9d6)==191,(word(0x9c2),word(0x9d6))
put(0x9c2,100);put(0x9d6,200);put(0x755,0x100);put(0x757,0)
press(256);wait(5);press(256)
health=word(0x9c2);reserve=word(0x9d6);wait(10)
assert (word(0x9c2),word(0x9d6))==(health,reserve) and health+reserve==300
# Changing category cancels, so reselecting does not resume a hidden transfer.
put(0x755,0x100);press(256);wait(3);put(0x755,1);step();assert word(0x757)==0
before=(word(0x9c2),word(0x9d6));put(0x755,0x100);wait(3);assert before==(word(0x9c2),word(0x9d6))
report['manualPreservesSurplus']=True;report['manualCancelAndReselect']=True
# Auto reserves: native state 27 executes the real patched transfer routine.
resume();put(0x9c0,1);put(0x9c2,1490);put(0x9d6,200);put(0x18a8,120)
put(0xa4e,50000);put(0xa50,12);put(0xa78,0x8000);put(0x998,27)
for _ in range(100):
 step()
 if l.sm_state()==8:break
assert l.sm_state()==8 and word(0x9c2)==1499 and word(0x9d6)==191,(l.sm_state(),word(0x9c2),word(0x9d6))
assert word(0x18a8)>=119,word(0x18a8)
report['autoPreservesSurplusAndInvincibility']=True
# Current seed objectives: original four bosses, no invented objectives.
# Kraid defeated in a controlled native flag fixture.
C.memmove(ram+0xd829,bytes([1]),1);step()
assert l.sm_varia_ui_state(3)==1 and l.sm_varia_ui_state(4)==300,(l.sm_varia_ui_state(3),l.sm_varia_ui_state(4))
capture('objective-notification');wait(300);assert l.sm_varia_ui_state(4)==0
report['objectiveNotificationFiveSeconds']=True
for area in (3,4,2):C.memmove(ram+0xd828+area,bytes([1]),1)
for index in (2,3,4,5):
 step();assert l.sm_varia_ui_state(3)==index
 if index==5:capture('all-objectives-notification')
 wait(300)
report['fourBossesAndAllObjectivesQueue']=True
assert l.sm_cpu_opcodes()==0
l.sm_shutdown();assert l.sm_seed_clear()
# Actual item PLM execution in Morph's room, with a reserve tank fixture.
morph=next(p for p in m['placements'] if p['location']=='Morphing Ball')
original_plm=morph['plm'];morph['plm']=0xef27
for enabled in (0,15):
 boot('SMTests-reserve-pickup-'+str(enabled),True);l.sm_varia_ui_configure(enabled)
 put(0x9d4,100);put(0x9d6,25)
 assert l.sm_test_seed_room(1)
 for _ in range(2000):
  step()
  if l.sm_state()==8 and l.sm_room()==0x9e9f:break
 wait(440);assert l.sm_test_seed_pickup_position()
 for _ in range(240):
  step(64)
  if word(0x9d4)==200:break
 assert word(0x9d4)==200 and word(0x9d6)==(200 if enabled else 25),(enabled,word(0x9d4),word(0x9d6))
 l.sm_shutdown();assert l.sm_seed_clear()
morph['plm']=original_plm
report['actualReservePickupFilled']=True
assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
report.update(passed=True,host=os.uname().nodename,sourceRomUnchanged=True,nativeSha256=hashlib.sha256(Path(l._name).read_bytes()).hexdigest())
(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('VARIA UI PASS',flush=True)
