#!/usr/bin/env python3
"""Capture native layer/tile provenance and reviewable original tile sheets."""
import ctypes as C
import json, hashlib
from pathlib import Path
import numpy as np
from PIL import Image,ImageDraw
root=Path(__file__).resolve().parents[1]
out=root/'Docs/LightingAudit'
out.mkdir(parents=True,exist_ok=True)
lib=C.CDLL(str(root/'Native/build/libsm_native.so'))
lib.sm_init.argtypes=[C.c_char_p,C.c_char_p]
for name in ('sm_pixels','sm_scene','sm_layers','sm_vram','sm_palette','sm_oam','sm_emission','sm_lightmap','sm_background_mask'):
    getattr(lib,name).restype=C.c_void_p
lib.sm_error.restype=C.c_char_p
lib.sm_cpu_opcodes.restype=C.c_uint64
lib.sm_set_parallax.argtypes=[C.c_int,C.c_int,C.c_int]
assert lib.sm_init(bytes(root/'roms/Super Metroid (Japan, USA) (En,Ja).sfc'),bytes(root/'.tmp/audit.sram'))

def capture(label):
    pixels=np.frombuffer(C.string_at(lib.sm_pixels(),256*240*4),np.uint8).reshape(240,256,4)
    meta=np.frombuffer(C.string_at(lib.sm_layers(),256*240*4),np.uint8).reshape(240,256,4)
    vram=C.string_at(lib.sm_vram(),65536)
    pal=np.frombuffer(C.string_at(lib.sm_palette(),512),'<u2')
    palette=np.stack([((pal>>k)&31)*255//31 for k in (0,5,10)],axis=1).astype(np.uint8)
    Image.fromarray(pixels[:224,:,[2,1,0]]).save(out/f'{label}.png')
    (out/f'{label}-vram.bin').write_bytes(vram)
    (out/f'{label}-palette.bin').write_bytes(C.string_at(lib.sm_palette(),512))
    (out/f'{label}-oam.bin').write_bytes(C.string_at(lib.sm_oam(),544))
    print("AUDIT",label,lib.sm_frame(),lib.sm_state(),"layers",np.unique(meta.reshape(-1,4),axis=0)[:12],flush=True)
    emission=np.frombuffer(C.string_at(lib.sm_emission(),256*240*4),np.uint8).reshape(240,256,4)
    Image.fromarray(emission[:224,:,[2,1,0]]).save(out/f'{label}-emission.png')
    light=np.frombuffer(C.string_at(lib.sm_lightmap(),64*60*4),np.uint8).reshape(60,64,4)
    Image.fromarray(light[:56,:,[2,1,0]]).resize((256,224)).save(out/f'{label}-lightmap.png')
    print('emitting pixels',np.count_nonzero(emission[:,:,:3].max(axis=2)),flush=True)
    mode=lib.sm_ppu_mode()
    entries=[]
    for layer in (0,1):
        rows=meta[:224][meta[:224,:,0]==layer]
        values=sorted(set((int(r[1])|((int(r[2])&3)<<8),(int(r[2])>>2)&7) for r in rows if r[3]))
        base=lib.sm_bg_info(layer,0)*2
        for tile,p in values:
            raw=vram[(base+tile*32)%65536:(base+tile*32)%65536+32]
            if mode==7: raw=bytes(vram[(tile*64+j)*2+1] for j in range(64))
            if len(raw) not in (32,64): continue
            idx=np.zeros((8,8),np.uint8)
            for y in range(8):
                for x in range(8):
                    idx[y,x]=sum(((raw[y*2+(b&1)+(b//2)*16]>>(7-x))&1)<<b for b in range(4))
            if mode==7: idx=np.frombuffer(raw,np.uint8).reshape(8,8)
            h=hashlib.sha256(raw).hexdigest()[:16]
            rgb=palette[p*16+idx]
            fnv=2166136261
            for v in raw: fnv=((fnv^v)*16777619)&0xffffffff
            entries.append({'layer':layer,'tile':tile,'palette':p,'hash':h,'fnv':f'{fnv:08x}','mode':mode,'rgb':rgb,'indices':idx})
    w=720; cellw=90; cellh=88
    sheet=Image.new('RGB',(w,max(1,(len(entries)+7)//8)*cellh),(20,23,28)); d=ImageDraw.Draw(sheet)
    for i,e in enumerate(entries):
        x=(i%8)*cellw;y=(i//8)*cellh
        sheet.paste(Image.fromarray(e['rgb']).resize((48,48),Image.Resampling.NEAREST),(x+20,y+4))
        d.text((x+3,y+55),f"B{e['layer']+1} {e['tile']:03x}/{e['palette']}",fill='white')
        d.text((x+3,y+69),e['hash'][:10],fill=(140,160,180))
    sheet.save(out/f'{label}-tiles.png')
    record={'label':label,'room':f'{lib.sm_room():04x}','frame':lib.sm_frame(),'mode':mode,'camera':[lib.sm_camera_x(),lib.sm_camera_y()],
            'samus':[lib.sm_samus_x(),lib.sm_samus_y()],
            'bg':[[lib.sm_bg_info(l,f) for f in range(6)] for l in range(2)],
            'tiles':[{k:v for k,v in e.items() if k not in ('rgb','indices')} for e in entries]}
    (out/f'{label}.json').write_text(json.dumps(record,indent=2)+'\n')
    print(label,record['room'],record['camera'],record['samus'],len(entries),flush=True)

for frame in range(8550):
    assert lib.sm_step(8 if frame>180 and frame%120<2 else 0),lib.sm_error()
capture('ceres-elevator')
for frame in range(550):
    assert lib.sm_step((128|512) if frame<160 else 0),lib.sm_error()
capture('ceres-walk')
# Same real frame, original PPU versus the extracted decorative plane.
lib.sm_set_parallax(2,0,1)
for _ in range(4):assert lib.sm_step(0)
original=np.frombuffer(C.string_at(lib.sm_pixels(),256*240*4),np.uint8).reshape(240,256,4)[:224,:,:3]
scene=np.frombuffer(C.string_at(lib.sm_scene(),256*240*4),np.uint8).reshape(240,256,4)[:224,:,:3]
meta=np.frombuffer(C.string_at(lib.sm_layers(),256*240*4),np.uint8).reshape(240,256,4)[:224]
changed=np.any(original!=scene,axis=2)
assert changed.sum()>500, 'Ceres checker plane did not move'
assert not changed[:32].any(), 'HUD shifted'
assert not changed[(meta[:,:,0]==4)|(meta[:,:,0]==6)].any(), 'Sprite shifted'
mask=np.frombuffer(C.string_at(lib.sm_background_mask(),256*240),np.uint8).reshape(240,256)[:224]>0
assert not changed[~mask].any(), 'Pixels outside the authored background mask changed'
Image.fromarray((mask*255).astype(np.uint8)).resize((768,672),Image.Resampling.NEAREST).save(out/'ceres-background-mask.png')
Image.fromarray(original[:,:,::-1]).resize((768,672),Image.Resampling.NEAREST).save(out/'ceres-parallax-reference.png')
Image.fromarray(scene[:,:,::-1]).resize((768,672),Image.Resampling.NEAREST).save(out/'ceres-parallax-shifted.png')
report={'room':hex(lib.sm_room()),'frame':lib.sm_frame(),'changed_background_pixels':int(changed.sum()),'hud_unchanged':True,'sprites_unchanged':True,'metal_frame_pixels_unchanged':True,'cpu_opcodes':lib.sm_cpu_opcodes()}
(out/'parallax-verification.json').write_text(json.dumps(report,indent=2)+'\n')
print(report,flush=True)
print('cpu_opcodes',lib.sm_cpu_opcodes())
lib.sm_shutdown()
