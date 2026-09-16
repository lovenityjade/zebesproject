#!/usr/bin/env python3
"""Exercise actual native boot/menus; reject every 65816 CPU fallback."""
import ctypes as C
import hashlib
import json
import argparse
from pathlib import Path
from PIL import Image

parser = argparse.ArgumentParser()
parser.add_argument('--gameplay', action='store_true')
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
out = root / '.tmp/native-test'
out.mkdir(parents=True, exist_ok=True)
rom = root / 'roms/Super Metroid (Japan, USA) (En,Ja).sfc'
assert hashlib.sha1(rom.read_bytes()).hexdigest() == 'da957f0d63d14cb441d215462904c4fa8519c613'
lib = C.CDLL(str(root / 'Native/build/libsm_native.so'))
lib.sm_init.argtypes = [C.c_char_p, C.c_char_p]
lib.sm_step.argtypes = [C.c_uint16]
lib.sm_error.restype = C.c_char_p
lib.sm_pixels.restype = C.POINTER(C.c_uint8)
lib.sm_audio.restype = C.POINTER(C.c_int16)
lib.sm_cpu_opcodes.restype = C.c_uint64
assert lib.sm_init(bytes(rom), bytes(out / 'sram.dat')), lib.sm_error()
transitions = []
last = -1
peak = 0
limit = 18000 if args.gameplay else 4200
for frame in range(limit):
    buttons = 8 if frame > 180 and frame % 120 < 2 else 0
    assert lib.sm_step(buttons), (frame, lib.sm_error())
    assert lib.sm_cpu_opcodes() == 0
    state = lib.sm_state()
    if state != last:
        transitions.append({'frame': frame, 'state': state, 'room': lib.sm_room(), 'x': lib.sm_samus_x(), 'y': lib.sm_samus_y()})
        print(transitions[-1], flush=True)
        last = state
    samples = C.string_at(lib.sm_audio(), 736 * 4)
    peak = max(peak, max(abs(x) for x in C.cast(lib.sm_audio(), C.POINTER(C.c_int16 * 1472)).contents))
    if frame in (180, 600, 1800, 3000, 4199) or (args.gameplay and state == 8):
        Image.frombytes('RGBA', (256,240), C.string_at(lib.sm_pixels(), 256*240*4), 'raw', 'BGRA').crop((0,0,256,224)).save(out / f'frame-{frame}.png')
    if args.gameplay and state == 8:
        (out / 'gameplay-frame.txt').write_text(str(frame + 1))
        break
assert peak > 0, 'Audio remained silent'
assert len(transitions) > 2, 'Did not advance beyond boot'
assert lib.sm_save()
assert not args.gameplay or last == 8, 'Did not reach actual gameplay'
report = {'frames':frame+1,'cpu_opcodes':lib.sm_cpu_opcodes(),'audio_peak':peak,'transitions':transitions,'core_only':True}
(out/'report.json').write_text(json.dumps(report,indent=2)+'\n')
lib.sm_shutdown()
print(json.dumps(report),flush=True)
