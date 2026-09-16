#!/usr/bin/env python3
"""Compare actual Vulkan pause screenshots with the native UI of that frame."""
import json
import shutil
import sys
from pathlib import Path
import numpy as np
from PIL import Image
root=Path(__file__).resolve().parent.parent
source=Path(sys.argv[1]) if len(sys.argv)>1 else root/'Docs/NativePause/GamingPC'
out=root/'Docs/NativePause/GamingPC';out.mkdir(parents=True,exist_ok=True)
rows=[]
for t in (100,200,350,520):
    stem=f'native-pause-{t}'
    raw=np.fromfile(source/f'{stem}-ui.bgra',np.uint8).reshape(240,400,4)[:224]
    expected=np.repeat(np.repeat(raw[:,:,[2,1,0]],3,axis=0),3,axis=1)
    mask=np.repeat(np.repeat(raw[:,:,3]>0,3,axis=0),3,axis=1)
    capture=np.array(Image.open(source/f'{stem}.png').convert('RGB'))
    assert capture.shape==(720,1280,3)
    actual=capture[24:696,40:1240]
    error=np.abs(actual.astype(int)-expected.astype(int))[mask]
    assert mask.sum()>300000,'Missing native pause UI'
    assert error.max()<=1,(stem,'Native UI altered in Unreal',int(error.max()))
    rows.append(dict(capture=stem,uiPixels=int(mask.sum()),maxChannelError=int(error.max())))
    if source.resolve()!=out.resolve():
        for suffix in ('.png','-ui.bgra'):shutil.copy2(source/(stem+suffix),out/(stem+suffix))
report=json.loads((source/'native-pause-verification.json').read_text());assert report['passed']
if source.resolve()!=out.resolve():shutil.copy2(source/'native-pause-verification.json',out/'native-pause-verification.json')
(out/'pixel-verification.json').write_text(json.dumps(dict(passed=True,viewport=[1280,720],source=[400,224],origin=[40,24],scale=3,captures=rows),indent=2)+'\n')
print('SM_NATIVE_PAUSE_VULKAN_PIXELS_PASS',len(rows),'captures')
