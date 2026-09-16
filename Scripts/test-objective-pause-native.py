#!/usr/bin/env python3
"""Real original pause inputs/transitions, VARIA objective tiles, native progress."""
import os,sys,struct
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-objectives.py').read_text().split("for goal in audit['goals']:")[0]
source=source.replace('objectives/libsm_native.so','objectives-pause/libsm_native.so').replace("root/'objectives/native'","root/'objectives-pause/native'")
exec(compile(source,'objective-pause-fixture','exec'))
out=root/'objectives-pause/native';out.mkdir(exist_ok=True)
l.sm_objective_pause_state.argtypes=[C.c_int]
ui=l.sm_objective_pause_state
art=json.loads((root/'objectives-pause/assets.json').read_text())
footer_checks=0
def verify_footer():
    global footer_checks
    for original,tile in art['footerRelocations']:
        expected=C.string_at(romptr+0x1b0000+original*32,32)
        assert C.string_at(l.sm_vram()+tile*32,32)==expected,(original,tile)
        footer_checks+=1
    # Real L/R ovals and EXIT/START glyphs, with flips preserved. Their
    # relocation must not turn into a second mapping on subsequent frames.
    reloc=dict(art['footerRelocations'])
    for y in [25,26]:
        for x in [1,2,3,4,12,13,14,15,16,17,18,19,27,28,29,30]:
            original=int.from_bytes(C.string_at(romptr+0x1b6000+2*(y*32+x),2),'little')
            actual=int.from_bytes(C.string_at(l.sm_vram()+0x7000+2*(y*32+x),2),'little')
            assert (actual&0xc3ff)==((original&0xc000)|reloc.get(original&1023,original&1023)),(y,x,hex(original),hex(actual))
            footer_checks+=1
def until(condition,label):
    for _ in range(240):
        step()
        if condition():return
    raise AssertionError((label,l.sm_state(),word(0x727),word(0x763),ui(0)))
def open_map():
    press(8);until(lambda:l.sm_state()==15 and word(0x727)==0,'map');wait(5)
def held(button):
    for _ in range(8):step(button)
    step()
def open_goals():
    held(1024);until(lambda:ui(3),'objectives');wait(5)
    verify_footer()
def map_back():
    held(2048);until(lambda:l.sm_state()==15 and word(0x727)==0,'return-map');wait(5)
def resume():
    held(8);until(lambda:l.sm_state()==8,'unpause');wait(5)
def state_words():return tuple(word(a) for a in [0x9a4,0x9a8,0x9c2,0x9c4,0x9c6,0x9c8,0x9ca,0x9cc,0x9ce,0x9d0,0x79b,0x79f])
goals=['tickle the red fish','collect 25% items','activate chozo robots']
p=plan([byname[n] for n in goals],required=2,flags=3);commit(p)
before_game=state_words();open_map();map_scroll=(word(0xb1),word(0xb3))
map_gfx=C.string_at(l.sm_vram(),0x4000);open_goals()
assert word(0x763)==2 and word(0x753)==3 and ui(0)==1
capture('hidden-wide')
native=Image.frombytes('RGBA',(256,240),C.string_at(l.sm_pixels(),256*240*4),'raw','BGRA');native.crop((0,0,256,224)).resize((768,672),Image.Resampling.NEAREST).save(out/'hidden-native.png')
hidden=C.string_at(l.sm_vram()+0x6000,4096)
for _ in range(5):press(32)
assert ui(1)==0,'Hidden goals leaked through scrolling'
map_back();assert (word(0xb1),word(0xb3))==map_scroll
assert C.string_at(l.sm_vram(),0x4000)==map_gfx,'Objective glyphs leaked into restored map'
assert state_words()==before_game
# Right equipment / left map still use their original state machines.
held(2048);until(lambda:l.sm_state()==15 and word(0x727)==1,'equipment');wait(5)
capture('equipment-after-objectives')
held(1024);until(lambda:l.sm_state()==15 and word(0x727)==0,'equipment-map');wait(5)
# Select retains the original file-load region overview rather than L.
press(4);assert l.sm_map_browser_state(0)==1;press(1);wait(5);assert l.sm_map_browser_state(0)==0
resume();assert ui(0)==0
mark(161);mark(audit['goals'][byname[goals[0]]]['arg'])
for i in range(12):collect(i)
for _ in range(6):frame()
open_map();open_goals();capture('revealed-progress-wide')
native=Image.frombytes('RGBA',(256,240),C.string_at(l.sm_pixels(),256*240*4),'raw','BGRA');native.crop((0,0,256,224)).resize((768,672),Image.Resampling.NEAREST).save(out/'revealed-progress-native.png')
assert C.string_at(l.sm_vram()+0x6000,4096)!=hidden
assert value(0,0)==1 and value(1,1)==12 and state(3)==1
resume();assert ui(0)==0 and l.sm_cpu_opcodes()==0
# Eighteen supported goals scroll through all entries, with native button
# repeats bounded at both ends and no unintended equipment/room changes.
many=[g['id'] for g in audit['goals'] if g['supported']][:18]
p=plan(many,required=9);commit(p);open_map();open_goals()
assert ui(1)==0 and ui(2)==5
for _ in range(24):press(32)
assert ui(1)==12 and ui(2)==17,(ui(1),ui(2))
capture('last-six-wide')
for _ in range(24):press(16)
assert ui(1)==0 and ui(2)==5
map_back();resume()
# Source progress rules: regional counts start only after its native start
# event; family counts start after a first kill. Both differ from always 0/N.
p=plan([byname['clear green brinstar'],byname['kill all beetoms']]);commit(p)
C.memset(ram+0xd91c+0x180,0,37);C.memset(ram+0xd870,0,64)
open_map();open_goals()
def goal_words(row):return list(struct.unpack('<32H',C.string_at(l.sm_vram()+0x6000+64*(row+5),64)))
before_region,before_family=goal_words(6),goal_words(8)
assert before_region[2]==before_family[2]==0
mark(149)
beetom=next(e for e in event_audit['enemies'] if e['type']==audit['goals'][byname['kill all beetoms']]['arg'])
mark(beetom['event']);wait(3)
after_region,after_family=goal_words(6),goal_words(8)
assert after_region[2]==after_family[2]==0x2577
assert after_region!=before_region and after_family!=before_family
capture('regional-family-progress-wide')
# Start wins over a simultaneous shoulder press and restores all resources.
held(8|2048);until(lambda:l.sm_state()==8,'start-with-right');wait(5)
assert ui(0)==0
# ABI inactive/legacy path does not introduce a page or consume L on map.
assert activate(0,0);assert select(None,0,None);open_map();held(1024);wait(20)
assert word(0x727)==0 and word(0x763)==0 and ui(0)==0
resume();assert l.sm_cpu_opcodes()==0
(out/'verification.json').write_text(json.dumps(dict(passed=True,hidden=True,revealed=True,scroll18=True,
    originalEquipmentNavigation=True,regionBrowser=True,returnScrollPreserved=True,unpause=True,vanilla=True,
    originalFooterChecks=footer_checks,mapGraphicsRestored=True,regionalFamilyProgress=True,startPriority=True,
    nativeSha256=hashlib.sha256((root/'objectives-pause/libsm_native.so').read_bytes()).hexdigest()),indent=2)+'\n')
l.sm_shutdown();print('OBJECTIVE_PAUSE_NATIVE_PASS',flush=True)
