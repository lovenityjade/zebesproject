#!/usr/bin/env python3
import json,shutil,sys
from pathlib import Path
import numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parent.parent
sys.path.insert(0,str(ROOT/'Tests'))
from gaussian_reference import compose
source=ROOT/'Unreal/Saved/SMTests';out=ROOT/'Docs/DisplayFixes/Unreal';out.mkdir(parents=True,exist_ok=True)
raw=np.fromfile(source/'story-scene.bgra',np.uint8).reshape(240,400,4)[:224,:,[2,1,0]]
ui=np.fromfile(source/'story-ui.bgra',np.uint8).reshape(240,400,4)[:224,:,[2,1,0,3]]
mask=ui[:,:,3].repeat(3,0).repeat(3,1)>0
ui_rgb=ui[:,:,:3].repeat(3,0).repeat(3,1)
expected=compose(raw,3,.75,1.10,hud_top=0);expected[mask]=ui_rgb[mask]
actual=np.array(Image.open(source/'Unreal-native.png').convert('RGB'))[24:696,40:1240]
error=np.abs(actual.astype(int)-expected.astype(int))
assert mask.sum()>1000,'Story text not separated'
assert error[mask].max()<=1,('Story text changed',int(error[mask].max()))
assert error.max()<=4 and error.mean()<.25,('Cinematic Gaussian mismatch',int(error.max()),float(error.mean()))
original=raw.repeat(3,0).repeat(3,1);original[mask]=ui_rgb[mask]
changed=np.any(np.abs(actual.astype(int)-original.astype(int))>4,axis=2)&~mask
assert changed.sum()>1000,'Cinematic atmosphere missing'
shutil.copy2(source/'Unreal-native.png',out/'story.png')
for name in ('story-scene.bgra','story-ui.bgra'):shutil.copy2(source/name,out/name)
report=dict(passed=True,width=400,scale=3,protectedTextPixels=int(mask.sum()),textMaxError=int(error[mask].max()),gaussianMaxError=int(error.max()),gaussianMeanError=float(error.mean()),atmosphereChangedPixels=int(changed.sum()))
(out/'story-pixel-verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('SM_STORY_GPU_PASS',json.dumps(report))
