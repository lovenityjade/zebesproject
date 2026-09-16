#!/usr/bin/env python3
"""Compare actual Vulkan captures, protecting exact native GUI pixels."""
import json,sys
from pathlib import Path
import numpy as np
from PIL import Image
root=Path(sys.argv[1]);a=np.array(Image.open(root/'credits-original.png').convert('RGB'));b=np.array(Image.open(root/'credits-glow.png').convert('RGB'));c=np.array(Image.open(root/'credits-twinkle.png').convert('RGB'))
assert a.shape==b.shape==c.shape
ui=np.frombuffer((root/'credits-original-ui.bgra').read_bytes(),dtype=np.uint8).reshape(240,400,4)[:224]
h,w=a.shape[:2];scale=min(w//400,h//224);x=(w-400*scale)//2;y=(h-224*scale)//2
mask=np.repeat(np.repeat(ui[:,:,3]>0,scale,axis=0),scale,axis=1)
a=a[y:y+224*scale,x:x+400*scale];b=b[y:y+224*scale,x:x+400*scale];c=c[y:y+224*scale,x:x+400*scale]
assert mask.sum()>2000 and np.array_equal(a[mask],b[mask]),'GUI changed under atmosphere'
delta=np.abs(a.astype(int)-b.astype(int));assert (delta[~mask]>3).sum()>5000
# Top sky strip contains neither moving credit text nor planet.
assert np.count_nonzero(b[:25*scale]!=c[:25*scale])>10,'No twinkle animation'
report=dict(passed=True,protectedGuiPixels=int(mask.sum()),guiByteExact=True,atmosphereChangedChannels=int((delta[~mask]>3).sum()),animatedStarChannels=int(np.count_nonzero(b[:25*scale]!=c[:25*scale])),viewport=[w,h],sourceSize=[400,224],scale=scale)
(root/'visual-verification.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
