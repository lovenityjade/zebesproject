#!/usr/bin/env python3
"""Validate ROM extraction and decoration-only changes against native gameplay."""
import ctypes as C
import hashlib
import json
import shutil
import socket
import sys
import tempfile
from pathlib import Path
from PIL import Image
assert socket.gethostname() == 'gaming-pc', 'Run this validation on gaming-pc only'
root=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(root/'Tools/RoomEditor'))
from assets import Assets
assets=Assets();out=root/'Docs/RoomEditor';out.mkdir(parents=True,exist_ok=True)
errors=[];states=0
for room in assets.rooms.values():
    if not room['supported']:continue
    for state in room['states']:
        try:assets.room(room['id'],state['id']);states+=1
        except Exception as error:errors.append(dict(room=hex(room['id']),state=hex(state['id']),error=str(error)))
print('ROM catalog',len(assets.rooms),'rooms',states,'states',errors,flush=True)
assert not errors,errors
lib=C.CDLL(str(root/'Native/build/libsm_native.so'));lib.sm_init.argtypes=[C.c_char_p,C.c_char_p]
for n in ('sm_simulation_ram','sm_pixels','sm_wide_scene','sm_scene'):getattr(lib,n).restype=C.c_void_p
lib.sm_decor_get.argtypes=[C.c_int,C.c_int,C.c_int,C.POINTER(C.c_uint16)]
def step(b=0):assert lib.sm_step(b)
def ram(a):return C.c_uint16.from_address(lib.sm_simulation_ram()+a).value
baseline=[];wide_before=None;definition_checks=0;patch=None;scenes=[];visual_checks=[];fixture_cells=[]
with tempfile.TemporaryDirectory(prefix='SMTests-decor-') as tmp:
    for mode in (0,1,2):
        enabled=mode!=0
        lib.sm_decor_clear()
        save=Path(tmp)/f'{mode}.sram';shutil.copy2(root/'Unreal/Saved/SMTests/HUD-all-items.sram',save)
        assert lib.sm_init(bytes(root/'roms/Super Metroid (Japan, USA) (En,Ja).sfc'),bytes(save))
        lib.sm_set_widescreen(1);lib.sm_set_border_extension(0)
        for _ in range(8500):
            f,s=lib.sm_frame(),lib.sm_state();step(8 if f>180 and f%120<2 and not 7<=s<=18 else 0)
            if lib.sm_state()==8:break
        for _ in range(440):step()
        assert lib.sm_test_awaken();assert lib.sm_test_room(0x9f11,39,139)
        for _ in range(440):step()
        state=ram(0x7bb);room=assets.room(0x9f11,state);definitions=assets.tileset(room['tileset'])['definitions']
        live=C.string_at(lib.sm_simulation_ram()+0xa000,8192)
        for tile in room['used']:
            assert definitions[tile*8:tile*8+8]==live[tile*8:tile*8+8],('tile definition',tile)
            definition_checks+=1
        if enabled:
            patch=assets.validate_patch(dict(room=room['id'],state=state,cells=[
                dict(x=-1,y=8,layer=0,tile=room['foreground'][9*room['width']+1]&4095),
                dict(x=2,y=9,layer=0,tile=room['foreground'][10*room['width']+1]&4095)] if mode==1 else [
                dict(x=-1,y=8,layer=1,tile=room['foreground'][9*room['width']+1]&4095),
                dict(x=2,y=8,layer=1,tile=room['foreground'][10*room['width']+1]&4095)]))
            for c in patch['cells']:
                # Use the current native baseline for this fixture (a PLM can
                # already have changed the authored level's interactive cell).
                if c['x']>=0 and c['expected']>=0:c['expected']=ram(0x10002+2*(c['y']*room['width']+c['x'])+(0x9600 if c['layer'] else 0))
                assert lib.sm_decor_set(room['id'],state,room['tileset'],c['layer'],c['x'],c['y'],c['tile'],c['expected'])
            fixture_cells.extend(patch['cells'])
            if mode==1:
                value=C.c_uint16()
                assert lib.sm_decor_get(0,2,9,C.byref(value))
                address=lib.sm_simulation_ram()+0x10002+2*(9*room['width']+2)
                cell=C.c_uint16.from_address(address);old=cell.value;cell.value^=1
                assert not lib.sm_decor_get(0,2,9,C.byref(value)),'Paint conceals a changed native block'
                cell.value=old
            assert not lib.sm_decor_set(room['id'],state,room['tileset'],2,0,0,1,-1)
        for t in range(180):
            if enabled and t==90:lib.sm_set_widescreen(0)
            step(128 if 30<=t<60 else 0)
            digest=hashlib.sha256(C.string_at(lib.sm_simulation_ram(),131072)+C.string_at(lib.sm_pixels(),256*240*4)).hexdigest()
            if not enabled:baseline.append(digest)
            else:assert baseline[t]==digest,('Simulation/native frame changed',t)
            if t==0:
                pixels=C.string_at(lib.sm_wide_scene(),400*240*4)
                Image.frombytes('RGBA',(400,240),pixels,'raw','BGRA').crop((0,0,400,224)).save(out/f'decor-{mode}.png')
                if not enabled:wide_before=pixels
                else:assert sum(a!=b for i,(a,b) in enumerate(zip(wide_before,pixels)) if i%4!=3)>100,'No visible paint'
            scene=C.string_at(lib.sm_scene(),256*240*4)
            if not enabled:scenes.append(scene)
            elif t in (0,90,179):
                wide=C.string_at(lib.sm_wide_scene(),400*240*4)
                center=b''.join(wide[(y*400+72)*4:(y*400+328)*4] for y in range(32,224))
                assert scene[32*1024:224*1024]==center,('4:3 center mismatch',mode,t)
                changed=sum(a!=b for i,(a,b) in enumerate(zip(scene[32*1024:224*1024],scenes[t][32*1024:224*1024])) if i%4!=3)
                assert changed>10,('No visible interior paint',mode,t)
                visual_checks.append(dict(layer=mode-1,frame=t,widescreen=t<90,changedRgbChannels=changed))
        assert not lib.sm_cpu_opcodes();lib.sm_shutdown()
lib.sm_decor_clear()
try:assets.validate_patch(dict(room=0x9f11,state=state,cells=[],collision=[]))
except ValueError:pass
else:raise AssertionError('Collision editing accepted')
report=dict(passed=True,rooms=len(assets.rooms),roomStates=states,tileDefinitionChecks=definition_checks,
            comparedFrames=len(baseline),nativePixelsAndRamExact=True,changedBlockGuard=True,collisionEditsRejected=True,visualChecks=visual_checks,foregroundAndBackground=True,nativeAspectRetouches=True)
(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
patch['cells']=fixture_cells
(out/'fixture.json').write_text(json.dumps(patch,indent=2)+'\n')
print('SM_ROOM_EDITOR_CORE_PASS',report)
