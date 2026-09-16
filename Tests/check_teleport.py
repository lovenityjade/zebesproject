#!/usr/bin/env python3
"""Traverse every curated native load-station destination on a copied save."""
from pathlib import Path
import ctypes as C,json,shutil,time,hashlib
from PIL import Image
root=Path(__file__).resolve().parents[1];out=root/'Docs/Teleport';out.mkdir(parents=True,exist_ok=True)
source=root/'Unreal/Saved/SMPreview/sram.dat';before=source.read_bytes()
copy=root/'.tmp'/f'teleport-{time.time_ns()}.sram';shutil.copy2(source,copy)
lib=C.CDLL(str(root/'Native/build/libsm_native.so'))
lib.sm_init.argtypes=[C.c_char_p,C.c_char_p];lib.sm_error.restype=C.c_char_p
lib.sm_teleport_name.restype=C.c_char_p;lib.sm_pixels.restype=C.c_void_p;lib.sm_cpu_opcodes.restype=C.c_uint64
assert lib.sm_init(bytes(root/'roms/Super Metroid (Japan, USA) (En,Ja).sfc'),bytes(copy)),lib.sm_error()
assert lib.sm_teleport(-1)==0 and lib.sm_teleport(999)==0
for _ in range(8500):
    f=lib.sm_frame();s=lib.sm_state()
    assert lib.sm_step(8 if f>180 and f%120<2 and not 7<=s<=18 else 0),lib.sm_error()
    if lib.sm_state()==8:break
assert lib.sm_state()==8
rows=[]
for destination in list(range(lib.sm_teleport_count()))+[0]:
    assert lib.sm_teleport(destination),('request rejected',destination,lib.sm_state())
    assert lib.sm_teleport(0)==0,'Must reject requests during loading'
    for f in range(600):
        assert lib.sm_step(0),lib.sm_error()
        if lib.sm_state()==8:break
    assert lib.sm_state()==8 and lib.sm_room()==lib.sm_teleport_room(destination),(destination,lib.sm_state(),hex(lib.sm_room()))
    for _ in range(30):assert lib.sm_step(0),lib.sm_error()
    assert lib.sm_state()==8 and lib.sm_cpu_opcodes()==0
    row={'destination':destination,'name':lib.sm_teleport_name(destination).decode(),'room':hex(lib.sm_room()),'load_frames':f+1,'fx':lib.sm_fx_type(),'water_y':lib.sm_water_y(),'heated':lib.sm_heated_room()}
    rows.append(row);print(row,flush=True)
    Image.frombytes('RGBA',(256,240),C.string_at(lib.sm_pixels(),256*240*4),'raw','BGRA').crop((0,0,256,224)).save(out/f'destination-{destination}.png')
lib.sm_shutdown()
assert source.read_bytes()==before,'Player save modified'
assert copy.read_bytes()==before,'Teleport wrote progression into SRAM'
report={'destinations':rows,'native_cpu_opcodes':0,'player_save_unchanged':True,'sram_not_written_by_teleport':True,'player_save_sha256':hashlib.sha256(before).hexdigest()}
(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
