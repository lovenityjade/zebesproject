#!/usr/bin/env python3
import json
from pathlib import Path
import numpy as np
from PIL import Image
from gaussian_reference import compose
root=Path(__file__).resolve().parents[1];out=root/'Unreal/Saved/SMTests'
far=np.frombuffer((out/'depth-mask.bin').read_bytes(),np.uint8).reshape(240,256)>0
raw=[np.frombuffer((out/f'depth-{name}.bgra').read_bytes(),np.uint8).reshape(240,256,4) for name in ['flat','layered']]
changed=np.any(raw[0]!=raw[1],axis=2)
assert changed.sum()>500,'No depth shading on real Ceres backdrop'
assert not changed[~far].any(),'Depth altered source foreground or GUI texels'
assert not changed[:32].any(),'Depth altered HUD'
state=json.loads((out/'depth-state.json').read_text());assert state['state']==8 and state['cpu_opcodes']==0
report={'native':state,'test':'Same frozen native frame in Unreal, flat vs layered, Gaussian Lighten preserved',
        'opacity':.75,'exposure':1.10,'modified_source_texels':int(changed.sum()),'source_foreground_and_gui_unchanged':True,'captures':{}}
for n,source in zip(['flat','layered'],raw):
    shot=np.asarray(Image.open(out/f'depth-{n}.png').convert('RGB'))
    h,w,_=shot.shape;scale=min(w//256,h//224);x,y=(w-256*scale)//2,(h-224*scale)//2
    crop=shot[y:y+224*scale,x:x+256*scale]
    expected=compose(source[:224,:,[2,1,0]],scale,.75,1.10)
    error=np.abs(crop.astype(int)-expected.astype(int))
    assert error.max()<=2,f'Gaussian Lighten changed in {n}: {error.max()}'
    assert np.array_equal(crop[:32*scale],expected[:32*scale]),'GUI changed'
    report['captures'][n]={'reference_max_channel_error':int(error.max()),'reference_mean_error':float(error.mean())}
(root/'Docs/depth-verification.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
