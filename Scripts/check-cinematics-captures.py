#!/usr/bin/env python3
"""Check actual Vulkan cinematic additions independently of the Gaussian pass."""
import json,sys
from pathlib import Path
import numpy as np
from PIL import Image
root=Path(sys.argv[1]);samples=[]
for path in sorted(root.glob('cinema-*-original.png')):
 stem=path.name.removesuffix('-original.png')
 arrays=[np.array(Image.open(root/f'{stem}-{n}.png').convert('RGB')) for n in ('original','gaussian','effects','motion')]
 a,g,b,c=arrays
 assert a.shape==g.shape==b.shape==c.shape
 ui=np.frombuffer((root/f'{stem}-original-ui.bgra').read_bytes(),dtype=np.uint8).reshape(240,400,4)[:224]
 h,w=a.shape[:2];scale=min(w//400,h//224);x=(w-400*scale)//2;y=(h-224*scale)//2
 mask=np.repeat(np.repeat(ui[:,:,3]>0,scale,axis=0),scale,axis=1)
 a,g,b,c=[v[y:y+224*scale,x:x+400*scale] for v in arrays]
 assert np.array_equal(a[mask],g[mask]) and np.array_equal(a[mask],b[mask]),(stem,'GUI altered')
 delta=np.abs(g.astype(int)-b.astype(int));changed=int((delta[~mask]>3).sum())
 # The story portrait and early METROID 3 card contain no extra emitters.
 if stem.startswith('cinema-0-') or stem=='cinema-4-1000':assert changed==0,(stem,'Stray cinematic light on a text card')
 else:assert changed>10,(stem,'No visible cinematic enhancement')
 samples.append(dict(scene=stem,protectedGuiPixels=int(mask.sum()),guiByteExact=True,cinematicChangedChannels=changed,maximumCinematicDelta=int(delta.max()),motionChangedChannels=int(np.count_nonzero(b!=c))))
assert len(samples)>=6
report=dict(passed=True,renderer='Unreal Vulkan',testHost='gaming-pc',comparison='Original / Gaussian Lighten 110% / Gaussian plus cinematic effects / six native frames later',samples=samples)
(root/'cinematic-visual-verification.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
