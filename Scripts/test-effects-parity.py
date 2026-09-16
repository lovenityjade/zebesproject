#!/usr/bin/env python3
"""All 128 KiB of native RAM must agree every combat frame with effects on/off."""
import ctypes as C,json,shutil,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
lib=C.CDLL(str(ROOT/'Native/build/libsm_native.so'))
lib.sm_init.argtypes=[C.c_char_p,C.c_char_p];lib.sm_error.restype=C.c_char_p
lib.sm_simulation_hash.restype=C.c_uint64
lib.sm_simulation_ram.restype=C.c_void_p
source=ROOT/'Unreal/Saved/SMPreview/sram.dat';original=source.read_bytes();runs=[];bombFrames=[]
with tempfile.TemporaryDirectory(prefix='SMTests-effects-') as tmp:
 for enabled in (0,1):
  save=Path(tmp)/f'case-{enabled}.sram';shutil.copy2(source,save)
  assert lib.sm_init(bytes(ROOT/'roms/Super Metroid (Japan, USA) (En,Ja).sfc'),bytes(save)),lib.sm_error()
  lib.sm_set_assisted_walljump(0);lib.sm_set_widescreen(0)
  lib.sm_set_combat_effects(enabled);lib.sm_set_engine_weather(enabled)
  for _ in range(8500):
   f,s=lib.sm_frame(),lib.sm_state();assert lib.sm_step(8 if f>180 and f%120<2 and not 7<=s<=18 else 0),lib.sm_error()
   if lib.sm_state()==8:break
  assert lib.sm_teleport(0)
  for _ in range(600):
   assert lib.sm_step(0),lib.sm_error()
   if lib.sm_state()==8:break
  for _ in range(400):assert lib.sm_step(0),lib.sm_error()
  assert lib.sm_test_combat_equipment()
  samples=[];pb=[];explosions=0
  for t in range(850):
   b=128 if 120<=t<150 else 512 if 150<=t<220 or 270<=t<273 or 700<=t<703 else 32 if 240<=t<242 or 250<=t<252 else 4 if 260<=t<262 else 2 if 650<=t<652 else 0
   assert lib.sm_step(b),lib.sm_error()
   samples.append(C.string_at(lib.sm_simulation_ram(),131072))
   pb.append([lib.sm_visual_state(k,0) for k in (9,10,11,12,13)])
   explosions+=sum(lib.sm_visual_state(17,i)==24 for i in range(lib.sm_visual_state(14,0)))
  assert any(p[4]&0x8000 for p in pb) and explosions>0,(enabled,explosions)
  assert lib.sm_cpu_opcodes()==0
  runs.append(samples);bombFrames.append(pb);lib.sm_shutdown()
  print('PASS effects',enabled,'bomb events',explosions,flush=True)
assert runs[0]==runs[1],next((i for i,(a,b) in enumerate(zip(*runs)) if a!=b),None)
assert bombFrames[0]==bombFrames[1]
assert source.read_bytes()==original
out=ROOT/'Docs/Combat';out.mkdir(exist_ok=True)
(out/'native-parity.json').write_text(json.dumps(dict(passed=True,frames=850,nativeRamBytesComparedPerFrame=131072,powerBombStateIdentical=True,cpuOpcodes=0,userSaveUnchanged=True),indent=2)+'\n')
print('SM_EFFECTS_PARITY_PASS')
