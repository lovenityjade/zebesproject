#!/usr/bin/env python3
"""Reproduce the reported cuts with awake Zebes and private all-items saves.

Each on/off run follows identical inputs. Compare original frames and complete
simulation RAM at rest and during movement; capture the six reported rooms.
"""
import ctypes as C
import json
import shutil
import tempfile
from pathlib import Path
from PIL import Image

root = Path(__file__).resolve().parent.parent
out = root / 'Docs/VisualPause/Borders'
out.mkdir(parents=True, exist_ok=True)
lib = C.CDLL(str(root / 'Native/build/libsm_native.so'))
lib.sm_init.argtypes = [C.c_char_p, C.c_char_p]
lib.sm_error.restype = C.c_char_p
for name in ('sm_pixels', 'sm_wide_scene', 'sm_wide_overlay', 'sm_simulation_ram'):
    getattr(lib, name).restype = C.c_void_p
rooms = [(0x96ba,394,1803), (0x975c,39,139), (0x97b5,39,139),
         (0x9e9f,2008,651), (0x9f11,39,139), (0x9f64,39,651)]
baseline = {}
checks = []

def step(buttons=0):
    assert lib.sm_step(buttons), lib.sm_error()

def read(name, size):
    return C.string_at(getattr(lib, name)(), size)

with tempfile.TemporaryDirectory(prefix='SMTests-reported-borders-') as folder:
    for enabled in (0, 1):
        save = Path(folder) / f'{enabled}.sram'
        shutil.copy2(root / 'Unreal/Saved/SMTests/HUD-all-items.sram', save)
        assert lib.sm_init(bytes(root / 'roms/Super Metroid (Japan, USA) (En,Ja).sfc'), bytes(save))
        lib.sm_set_widescreen(1)
        lib.sm_set_border_extension(enabled)
        for _ in range(8500):
            frame, state = lib.sm_frame(), lib.sm_state()
            step(8 if frame > 180 and frame % 120 < 2 and not 7 <= state <= 18 else 0)
            if lib.sm_state() == 8:
                break
        for _ in range(440):
            step()
        assert lib.sm_test_awaken()
        for room, x, y in rooms:
            assert lib.sm_test_room(room, x, y)
            for _ in range(440):
                step()
            assert lib.sm_room() == room, (hex(room), hex(lib.sm_room()))
            for tick in range(121):
                if tick:
                    step(128 if tick < 61 else 64)
                key = (room, tick)
                state = (read('sm_pixels', 256*240*4), read('sm_simulation_ram', 131072))
                if not enabled:
                    baseline[key] = state
                else:
                    assert baseline[key] == state, ('native simulation/frame changed', key)
                if tick == 0:
                    scene = Image.frombytes('RGBA', (400,240), read('sm_wide_scene',400*240*4), 'raw','BGRA')
                    scene.alpha_composite(Image.frombytes('RGBA',(400,240),read('sm_wide_overlay',400*240*4),'raw','BGRA'))
                    scene.crop((0,0,400,224)).save(out / f'report-{room:04x}-{enabled}.png')
            if enabled:
                checks.append(dict(room=f'{room:04x}', comparedFrames=121, nativeExact=True))
        assert lib.sm_cpu_opcodes() == 0
        lib.sm_shutdown()
report = dict(passed=True, awakeZebes=True, rooms=checks)
(out / 'reported-verification.json').write_text(json.dumps(report, indent=2) + '\n')
print('SM_REPORTED_BORDERS_PASS', report)
