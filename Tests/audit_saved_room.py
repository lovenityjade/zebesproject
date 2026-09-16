#!/usr/bin/env python3
"""Read a copy of preview SRAM; inspect layer provenance in the saved room."""
import ctypes as C,shutil,json,time,sys
from pathlib import Path
import numpy as np
from PIL import Image
root=Path(__file__).resolve().parents[1];out=root/'Docs/Depth';out.mkdir(exist_ok=True)
copy=root/'.tmp'/f'depth-audit-{time.time_ns()}.sram'
shutil.copy2(root/'Unreal/Saved/SMPreview/sram.dat',copy)
lib=C.CDLL(str(root/'Native/build/libsm_native.so'));lib.sm_init.argtypes=[C.c_char_p,C.c_char_p]
for name in ['sm_pixels','sm_layers','sm_background_mask','sm_scene']:getattr(lib,name).restype=C.c_void_p
assert lib.sm_init(bytes(root/'roms/Super Metroid (Japan, USA) (En,Ja).sfc'),bytes(copy))
for frame in range(8500):
    assert lib.sm_step(8 if frame>180 and frame%120<2 and not 7<=lib.sm_state()<=18 else 0)
    if lib.sm_state()==8:break
if '--parlor' in sys.argv:
    for _ in range(1400):
        assert lib.sm_step(64|1|512)
        if lib.sm_room()==0x92fd and lib.sm_state()==8:break
for _ in range(60):assert lib.sm_step(0)
lib.sm_set_parallax(6,0,1)
for _ in range(4):assert lib.sm_step(0)
pixels=np.frombuffer(C.string_at(lib.sm_pixels(),256*240*4),np.uint8).reshape(240,256,4)
far=np.frombuffer(C.string_at(lib.sm_background_mask(),256*240),np.uint8).reshape(240,256)
layers=np.frombuffer(C.string_at(lib.sm_layers(),256*240*4),np.uint8).reshape(240,256,4)
Image.fromarray(pixels[:224,:,[2,1,0]]).resize((768,672),Image.Resampling.NEAREST).save(out/'saved-room.png')
Image.fromarray(far[:224]).resize((768,672),Image.Resampling.NEAREST).save(out/'saved-room-far.png')
colors=np.array([[200,80,60],[50,110,220],[50,170,70],[0,0,0],[255,200,50],[30,30,35],[255,200,50],[0,0,0]],np.uint8)
Image.fromarray(colors[layers[:224,:,0]&7]).resize((768,672),Image.Resampling.NEAREST).save(out/'saved-room-layers.png')
scene=np.frombuffer(C.string_at(lib.sm_scene(),256*240*4),np.uint8).reshape(240,256,4)
diff=np.any(scene[:224,:,:3]!=pixels[:224,:,:3],axis=2)
protected=(layers[:224,:,0]==0)|(layers[:224,:,0]==4)|(layers[:224,:,0]==6)
assert not diff[protected].any() and not diff[:32].any()
Image.fromarray(scene[:224,:,[2,1,0]]).resize((768,672),Image.Resampling.NEAREST).save(out/'saved-room-shifted.png')
report={'changed_background_pixels':int(diff.sum()),'foreground_and_hud_unchanged':True,'room' :hex(lib.sm_room()),'state':lib.sm_state(),'mode':lib.sm_ppu_mode(),'parallax_supported':lib.sm_parallax_supported(),'far_pixels':int((far>0).sum()),'layer_counts':dict(zip(*[a.tolist() for a in np.unique(layers[:224,:,0],return_counts=True)]))}
(out/'saved-room.json').write_text(json.dumps(report,indent=2)+'\n');print(report,flush=True)
lib.sm_shutdown()
