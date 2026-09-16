#!/usr/bin/env python3
"""Source HUD cadence, persistent notification bits and applied objective ranks."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-objectives.py').read_text().split("for goal in audit['goals']:")[0]
source=source.replace('objectives/libsm_native.so','objectives-pause/libsm_native.so').replace("root/'objectives/native'","root/'objectives-pause/hud'")
exec(compile(source,'objective-hud-fixture','exec'))
ui_frame=routine('sm_varia_ui_frame',None)
ui_reset=routine('sm_varia_ui_reset',None)
ui_render=routine('sm_varia_ui_render',None,C.c_void_p)
l.sm_varia_ui_configure.argtypes=[C.c_uint]
l.sm_varia_ui_state.argtypes=[C.c_int]
ui=l.sm_varia_ui_state
index=routine('sm_objectives_check_index',C.c_int)
many=[g['id'] for g in audit['goals'] if g['supported']][:18]
rank_results=[]
for rank in range(18):
    commit(plan(many,required=18,flags=2));clear();ui_reset();l.sm_varia_ui_configure(15)
    for _ in range(rank):frame()
    assert index()==rank
    # Completion is evidence distinct from merely fulfilling an interaction.
    mark(162+2*rank);ui_frame()
    assert ui(3)==rank+8 and ui(4)==300 and not event(163+2*rank)
    assert not state(7),'HUD notification must not reveal hidden goals'
    if rank==17:
        ui_render(l.sm_pixels());capture('objective-18')
    for _ in range(299):ui_frame()
    assert ui(3)==rank+8 and ui(4)==1 and not event(163+2*rank)
    ui_frame();assert ui(3)==ui(4)==0 and event(163+2*rank)
    ui_frame();assert ui(3)==0,'An acknowledged objective was announced again'
    rank_results.append(rank+1)

# Source all-required notification event is VARIA base $80, not an original
# game event. Optional goals and all-complete remain independent of quota.
names=['tickle the red fish','activate chozo robots','visit the animals']
p=plan([byname[n] for n in names],required=1,flags=2);commit(p);clear();ui_reset()
mark(10);ui_frame();assert ui(3)==5 and ui(4)==300 and not event(128)
ui_render(l.sm_pixels());capture('required-objectives')
put(0x998,15);ui_frame();assert ui(3)==0 and event(128) and not event(11)
assert state(4)==1 and state(5)==0 and not state(7)
mark(162);ui_frame();assert ui(3)==0,'Pause must not queue a new notification'
put(0x998,8);ui_frame();assert ui(3)==8
put(0x998,15);ui_frame();assert event(163) and not ui(3)
put(0x998,8)

# Disabling the HUD must not acknowledge information the player never saw.
commit(p);clear();ui_reset();mark(162);l.sm_varia_ui_configure(0)
for _ in range(400):ui_frame()
assert not event(163) and not ui(3)
l.sm_varia_ui_configure(15);ui_frame();assert ui(3)==8
l.sm_varia_ui_configure(0);ui_frame();assert not event(163) and not ui(3)
l.sm_varia_ui_configure(15);ui_frame();assert ui(3)==8

# Original checksummed SRAM retains each slot's acknowledged objectives.
patterns=[]
for slot,rank in enumerate([0,1,2]):
    commit(p,slot);clear();ui_reset()
    for _ in range(rank):frame()
    mark(162+2*rank);mark(10);ui_frame();assert ui(3)==8+rank
    put(0x998,15);ui_frame();put(0x998,8)
    ui_frame();assert ui(3)==5
    put(0x998,15);ui_frame();put(0x998,8)
    assert event(128) and event(163+2*rank)
    save(slot);patterns.append(read(0xd91c+0x180,37))
for slot in [2,0,1,2,1,0]:
    assert activate(slot,1) and select(items,100,m['sha256'].encode())
    clear();ui_reset();assert load(slot)==0
    assert read(0xd91c+0x180,37)==patterns[slot]
    for _ in range(6):frame();ui_frame()
    assert not ui(3),'Saved completion notifications were replayed'

# Exercise the public frame entry, not only the direct routine fixtures.
commit(p);clear();ui_reset();mark(audit['goals'][p.goals[0]]['arg'])
for _ in range(6):step()
assert value(0,0) and ui(3) in [5,8] and l.sm_cpu_opcodes()==0
assert activate(0,0) and select(None,0,None)
clear();ui_reset();before=read(0,131072);ui_frame()
assert ui(3)==0 and read(0,131072)==before
out=root/'objectives-pause/hud'
(out/'verification.json').write_text(json.dumps(dict(passed=True,ranks=rank_results,frames=300,
    hiddenNoReveal=True,quotaIndependent=True,pauseAcknowledgement=True,disabledHudNoAcknowledgement=True,
    sramSlots=3,reloads=6,realFrameEntry=True,vanilla=True,cpuOpcodes=l.sm_cpu_opcodes(),
    nativeSha256=hashlib.sha256((root/'objectives-pause/libsm_native.so').read_bytes()).hexdigest()),indent=2)+'\n')
l.sm_shutdown();print('OBJECTIVE_HUD_NATIVE_PASS',flush=True)
