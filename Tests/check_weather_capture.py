#!/usr/bin/env python3
"""Real Vulkan captures on a frozen native frame; forced-biome cases labelled."""
from pathlib import Path
import json
import os
import numpy as np
from PIL import Image
from gaussian_reference import compose
root=Path(__file__).resolve().parents[1];out=Path(os.environ.get('SM_CAPTURE_DIR',str(root/'Unreal/Saved/SMTests')))
state=json.loads((out/'weather-state.json').read_text())
assert state['state']==8 and state['cpu_opcodes']==0
raw=np.frombuffer((out/'weather-base.bgra').read_bytes(),np.uint8).reshape(240,256,4)
original=np.frombuffer((out/'weather-original.bgra').read_bytes(),np.uint8).reshape(240,256,4)
names=['baseline','rain','rain-moving','fog','water','heat','native','gui-bypass'];shots={}
for name in names:
    shot=np.asarray(Image.open(out/f'weather-{name}.png').convert('RGB'))
    h,w,_=shot.shape;scale=min(w//256,h//224);x,y=(w-256*scale)//2,(h-224*scale)//2
    shots[name]=shot[y:y+224*scale,x:x+256*scale]
base=shots['baseline'];expected=compose(raw[:224,:,[2,1,0]],scale,.75,1.10)
error=np.abs(base.astype(int)-expected.astype(int))
# Vulkan comparisons reach 3/255 in Maridia and 4/255 on one Norfair channel.
# Bound both worst-case quantization and global error; GUI remains byte-exact.
assert error.max()<=4 and error.mean()<.25,('Gaussian baseline mismatch',error.max(),error.mean())
report={'native':state,'gaussian_reference_max_error':int(error.max()),'gaussian_reference_mean_error':float(error.mean()),'cases':{},
        'note':'rain/fog/water/heat are isolated GPU previews on the same frame; native uses real environment flags.'}
for n in names[1:]:
    assert np.array_equal(shots[n][:32*scale],base[:32*scale]),f'HUD changed in {n}'
    delta=np.abs(shots[n].astype(int)-base.astype(int))
    changed=np.any(delta>2,axis=2)
    if n not in ['native','gui-bypass']:assert changed.sum()>500,f'Effect absent: {n}'
    report['cases'][n]={'changed_pixels':int(changed.sum()),'mean_channel_difference':float(delta.mean()),'hud_unchanged':True}
assert np.any(shots['rain']!=shots['rain-moving']),'Rain did not animate'
assert np.array_equal(shots['water'][:100*scale],base[:100*scale]),'Water effect leaked above its surface'
ref=np.repeat(np.repeat(original[:224,:,[2,1,0]],scale,axis=0),scale,axis=1)
assert np.array_equal(shots['gui-bypass'],ref),'Non-gameplay bypass is not original pixels'
report['rain_animated']=True;report['water_clipped_at_surface']=True;report['non_gameplay_bypassed']=True
if state['fx']==10 or state['water_y']!=32767 or state['heated']:
    assert report['cases']['native']['changed_pixels']>500,'Native biome did not trigger'
dest=root/'Docs/Weather'/state['room'];dest.mkdir(parents=True,exist_ok=True)
for n in names:(dest/f'{n}.png').write_bytes((out/f'weather-{n}.png').read_bytes())
(dest/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print(json.dumps(report,indent=2))
