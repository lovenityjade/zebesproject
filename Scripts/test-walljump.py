#!/usr/bin/env python3
"""Real native input sequences in the Crateria save room, private SRAM."""
import ctypes as C, hashlib, json, shutil, tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
lib=C.CDLL(str(ROOT/'Native/build/libsm_native.so'))
lib.sm_init.argtypes=[C.c_char_p,C.c_char_p];lib.sm_error.restype=C.c_char_p
source=ROOT/'Unreal/Saved/SMPreview/sram.dat';before=source.read_bytes()
rows=[]
with tempfile.TemporaryDirectory(prefix='sm-walljump-') as tmp:
 save=Path(tmp)/'private.sram';shutil.copy2(source,save)
 assert lib.sm_init(bytes(ROOT/'roms/Super Metroid (Japan, USA) (En,Ja).sfc'),bytes(save)),lib.sm_error()
 for _ in range(8500):
  f,s=lib.sm_frame(),lib.sm_state();assert lib.sm_step(8 if f>180 and f%120<2 and not 7<=s<=18 else 0),lib.sm_error()
  if lib.sm_state()==8:break
 for mode in (0,1):
  for side in (64,128):
   for held in (False,True):
    lib.sm_set_assisted_walljump(mode);assert lib.sm_assisted_walljump()==mode
    assert lib.sm_teleport(1)
    for _ in range(550):assert lib.sm_step(0),lib.sm_error()
    samples=[]
    for t in range(240):
     buttons=side | (256 if t>=20 and (held or (t-20)%24<12) else 0)
     assert lib.sm_step(buttons),lib.sm_error()
     samples.append([lib.sm_samus_x(),lib.sm_samus_y(),lib.sm_player_motion(0)])
    bounces=sum(s[2]==20 and (i==0 or samples[i-1][2]!=20) for i,s in enumerate(samples))
    if held or not mode:assert bounces==0,(mode,side,held,bounces)
    else:assert bounces>=2,(mode,side,held,bounces)
    rows.append(dict(assisted=bool(mode),side='left' if side==64 else 'right',heldJump=held,bounces=bounces,minY=min(s[1] for s in samples),samples=samples))
    print('PASS',mode,side,held,bounces,flush=True)
 assert lib.sm_teleport(0)
 for _ in range(550):assert lib.sm_step(0),lib.sm_error()
 ground=lib.sm_samus_y();air=[]
 for t in range(140):
  assert lib.sm_step(128 | (256 if 20<=t<50 or (t>=62 and (t-62)%12<6) else 0)),lib.sm_error()
  if lib.sm_samus_y()<ground-24:air.append(lib.sm_player_motion(0))
 assert len(air)>10 and 20 not in air,('Unexpected air bounce',air)
 assert lib.sm_cpu_opcodes()==0
 lib.sm_shutdown()
assert source.read_bytes()==before
out=ROOT/'Docs/WallJump';out.mkdir(exist_ok=True)
(out/'verification.json').write_text(json.dumps(dict(passed=True,scenarios=rows,noAirBounce=True,airFrames=len(air),cpuOpcodes=0,userSaveUnchanged=True),indent=2)+'\n')
print('SM_WALLJUMP_PASS')
