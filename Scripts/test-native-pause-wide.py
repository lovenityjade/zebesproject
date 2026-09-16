#!/usr/bin/env python3
"""Exercise the actual native map/equipment pause, with no Canvas UI."""
import ctypes as C
import hashlib
import json
import shutil
import socket
import tempfile
from pathlib import Path
from PIL import Image

assert socket.gethostname() == 'gaming-pc', 'Run this validation on gaming-pc only'
root=Path(__file__).resolve().parent.parent
out=root/'Docs/NativePause';out.mkdir(parents=True,exist_ok=True)
lib=C.CDLL(str(root/'Native/build/libsm_native.so'))
lib.sm_init.argtypes=[C.c_char_p,C.c_char_p];lib.sm_error.restype=C.c_char_p
for n in ('sm_pixels','sm_wide_scene','sm_wide_overlay','sm_simulation_ram'):
    getattr(lib,n).restype=C.c_void_p
baseline=[];captures=[]
def step(b=0):assert lib.sm_step(b),lib.sm_error()
def image(name,w):
    return Image.frombytes('RGBA',(w,240),C.string_at(getattr(lib,name)(),w*240*4),'raw','BGRA').crop((0,0,w,224))
with tempfile.TemporaryDirectory(prefix='SMTests-native-pause-') as tmp:
    for wide in (0,1):
        save=Path(tmp)/f'{wide}.sram'
        shutil.copy2(root/'Unreal/Saved/SMTests/HUD-all-items.sram',save)
        assert lib.sm_init(bytes(root/'roms/Super Metroid (Japan, USA) (En,Ja).sfc'),bytes(save))
        lib.sm_set_widescreen(wide);lib.sm_set_border_extension(0)
        for _ in range(8500):
            f,s=lib.sm_frame(),lib.sm_state();step(8 if f>180 and f%120<2 and not 7<=s<=18 else 0)
            if lib.sm_state()==8:break
        for _ in range(440):step()
        for t in range(720):
            b=8 if t<8 or 590<=t<598 else 2048 if 220<=t<228 else 1024 if 450<=t<458 else 128 if 130<=t<180 else 0
            step(b)
            digest=hashlib.sha256(C.string_at(lib.sm_simulation_ram(),131072)+C.string_at(lib.sm_pixels(),256*240*4)).hexdigest()
            if not wide:baseline.append(digest)
            else:assert digest==baseline[t],('Native behavior changed',t)
            if wide and t in (90,190,350,540):
                native=image('sm_pixels',256)
                scene=image('sm_wide_scene',400);scene.alpha_composite(image('sm_wide_overlay',400))
                native.save(out/f'pause-{t}-native.png');scene.save(out/f'pause-{t}-wide.png')
                assert scene.crop((136,32,264,48)).tobytes()==native.crop((64,32,192,48)).tobytes(),('Title pixels changed',t)
                assert lib.sm_state()==15
                assert lib.sm_pause_data(12)==(1 if t==350 else 0)
                if lib.sm_pause_data(12)==0:
                    # The horizontal arrows move to the expanded frame edges.
                    assert scene.crop((96,48,304,192)).tobytes()==native.crop((24,48,232,192)).tobytes(),('Map pixels changed',t)
                else:
                    for left,right,offset in ((8,96,0),(96,160,72),(160,248,144)):
                        assert scene.crop((left+offset,48,right+offset,192)).tobytes()==native.crop((left,48,right,192)).tobytes(),('Equipment pixels changed',t,left)
                captures.append(dict(tick=t,mode=lib.sm_pause_data(12),originalPixelsExact=True))
        assert lib.sm_state()==8 and not lib.sm_cpu_opcodes()
        lib.sm_shutdown()
report=dict(passed=True,nativeLifecycle=True,comparedFrames=len(baseline),captures=captures)
(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('SM_NATIVE_PAUSE_WIDE_PASS',report)
