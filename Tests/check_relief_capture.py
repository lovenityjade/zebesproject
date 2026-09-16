#!/usr/bin/env python3
"""Actual Unreal frozen-frame comparison; Gaussian checked independently."""
import json
from pathlib import Path
import numpy as np
from PIL import Image
from gaussian_reference import compose
root=Path(__file__).resolve().parents[1]
out=root/'Unreal/Saved/SMTests'
far=np.frombuffer((out/'depth-mask.bin').read_bytes(),np.uint8).reshape(240,256)>0
layers=np.frombuffer((out/'relief-layers.bgra').read_bytes(),np.uint8).reshape(240,256,4)
raw=[np.frombuffer((out/f'relief-{n}.bgra').read_bytes(),np.uint8).reshape(240,256,4) for n in ['flat','layered']]
changed=np.any(raw[0]!=raw[1],axis=2)
eligible=(layers[:,:,0]==0)&(layers[:,:,3]>0)&~far
eligible[:32]=False
eligible[224:]=False
assert changed.sum()>50,'Relief absent from real room'
assert not changed[~eligible].any(),'Relief changed sprites, GUI or far planes'
assert np.array_equal(raw[0][:,:,3],raw[1][:,:,3])
state=json.loads((out/'depth-state.json').read_text())
assert state['state']==8 and state['cpu_opcodes']==0
report={'native':state,'test':'Same frozen Unreal frame: depth with/without structure bevel',
        'opacity':.75,'exposure':1.10,'bevel_width':6,'edge_light_gain':.40,'edge_shadow_loss':.50,'modified_structure_texels':int(changed.sum()),
        'source_sprites_gui_far_planes_unchanged':True,'captures':{}}
for n,source in zip(['flat','layered'],raw):
    shot=np.asarray(Image.open(out/f'relief-{n}.png').convert('RGB'))
    h,w,_=shot.shape;scale=min(w//256,h//224);x,y=(w-256*scale)//2,(h-224*scale)//2
    crop=shot[y:y+224*scale,x:x+256*scale]
    expected=compose(source[:224,:,[2,1,0]],scale,.75,1.10)
    error=np.abs(crop.astype(int)-expected.astype(int))
    assert error.max()<=2,f'Gaussian mismatch in {n}: {error.max()}'
    assert np.array_equal(crop[:32*scale],expected[:32*scale]),'GUI changed'
    report['captures'][n]={'reference_max_channel_error':int(error.max()),'reference_mean_error':float(error.mean())}
dest=root/'Docs/Relief'/state['room'];dest.mkdir(parents=True,exist_ok=True)
for n in ['flat','layered']:(dest/f'{n}.png').write_bytes((out/f'relief-{n}.png').read_bytes())
(dest/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
