#!/usr/bin/env python3
"""Compare actual Vulkan screenshots with the native UI submitted that frame."""
import json,shutil,sys
from pathlib import Path
import numpy as np
from PIL import Image
ROOT=Path(__file__).resolve().parent.parent
source=Path(sys.argv[1]) if len(sys.argv)>1 else ROOT/'Unreal/Saved/SMTests'
out=ROOT/'Docs/DisplayFixes/Unreal';out.mkdir(parents=True,exist_ok=True)
rows=[]
for i in range(15):
 stem=f'display-{i:02d}'
 raw=np.fromfile(source/f'{stem}-ui.bgra',np.uint8).reshape(240,400,4)[:224]
 rgba=raw[:,:,[2,1,0,3]]
 mask=np.repeat(np.repeat(rgba[:,:,3]>0,3,axis=0),3,axis=1)
 expected=np.repeat(np.repeat(rgba[:,:,:3],3,axis=0),3,axis=1)
 capture=np.array(Image.open(source/f'{stem}.png').convert('RGB'))
 assert capture.shape==(720,1280,3)
 actual=capture[24:696,40:1240]
 assert mask.sum()>100,'Missing UI'
 error=np.abs(actual.astype(int)-expected.astype(int))[mask]
 assert error.max()<=1,(stem,'UI altered in Unreal',int(error.max()))
 rows.append(dict(capture=stem,uiPixels=int(mask.sum()),maxChannelError=int(error.max())))
 shutil.copy2(source/f'{stem}.png',out/f'{stem}.png')
 shutil.copy2(source/f'{stem}-ui.bgra',out/f'{stem}-ui.bgra')
runtime=json.loads((source/'display-verification.json').read_text());assert runtime['passed']
shutil.copy2(source/'display-verification.json',out/'runtime-verification.json')
(out/'pixel-verification.json').write_text(json.dumps(dict(passed=True,viewport=[1280,720],source=[400,224],origin=[40,24],scale=3,captures=rows),indent=2)+'\n')
print('SM_UNREAL_UI_PIXELS_PASS',len(rows),'captures')
