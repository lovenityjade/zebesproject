#!/usr/bin/env python3
"""Check actual Vulkan captures against a whole-image Gaussian layer reference."""
import json
from pathlib import Path
import numpy as np
from PIL import Image
root=Path(__file__).resolve().parents[1];out=root/'Unreal/Saved/SMTests'
raw=np.frombuffer((out/'reference.bgra').read_bytes(),np.uint8).reshape(240,256,4)[:224,:,[2,1,0]]
off=np.asarray(Image.open(out/'pixel-original.png').convert('RGB'))
lighten=np.asarray(Image.open(out/'pixel-atmosphere.png').convert('RGB'))
multiply=np.asarray(Image.open(out/'pixel-multiply.png').convert('RGB'))
h,w,_=off.shape;scale=min(w//256,h//224);x0,y0=(w-256*scale)//2,(h-224*scale)//2
base=raw.repeat(scale,0).repeat(scale,1)
original=off[y0:y0+224*scale,x0:x0+256*scale]
assert np.array_equal(original,base),'Effects-off colors/grid differ from original pixels'
# Reference the mathematical operation independently, including output subpixels.
# Every texel in the scene contributes, with no brightness/source selection.
px=(np.arange(256*scale)+.5)/scale;py=(np.arange(224*scale)+.5)/scale
cx=np.floor(px).astype(int);cy=np.floor(py).astype(int)
blur=np.zeros(base.shape,np.float64);weight_sum=np.zeros(base.shape[:2],np.float64)
for dy in range(-4,5):
    wy=np.exp(-((cy+dy+.5-py)**2)/(2*1.25**2))
    iy=np.clip(cy+dy,32,223)
    for dx in range(-4,5):
        wx=np.exp(-((cx+dx+.5-px)**2)/(2*1.25**2))
        weight=wy[:,None]*wx[None,:]
        sample=raw[iy[:,None],np.clip(cx+dx,0,255)[None,:]]/255.
        blur+=sample*weight[:,:,None];weight_sum+=weight
blur/=weight_sum[:,:,None]
b=base/255.;opacity=.35
predictions={'Lighten':b+opacity*(np.maximum(b,blur)-b),
             'Multiply':b+opacity*(b*blur-b)}
report={'presentation_version':3,'viewport':[w,h],'integer_scale':scale,
        'pixel_blocks_uniform':True,'maximum_channel_error':0,
        'gaussian_sigma_source_pixels':1.25,'overlay_opacity':opacity,'modes':{}}
for name,shot in [('Lighten',lighten),('Multiply',multiply)]:
    crop=shot[y0:y0+224*scale,x0:x0+256*scale]
    prediction=np.rint(predictions[name]*255).clip(0,255).astype(np.uint8)
    prediction[:32*scale]=base[:32*scale]
    error=np.abs(crop.astype(int)-prediction.astype(int))
    assert error.max()<=2, f'{name} does not match Gaussian layer composition: {error.max()}'
    assert np.array_equal(crop[:32*scale],base[:32*scale]),f'{name} altered GUI'
    outside=shot.copy();outside[y0:y0+224*scale,x0:x0+256*scale]=off[y0:y0+224*scale,x0:x0+256*scale]
    assert np.array_equal(outside,off),f'{name} altered margins'
    delta=crop.astype(int)-base.astype(int)
    if name=='Lighten':assert delta.min()>=-1,'Lighten darkened the image'
    else:assert delta.max()<=1,'Multiply brightened the image'
    changed=np.any(delta!=0,axis=2)
    # Confirm the effect extends well beyond reviewed lamp texels.
    assert changed[32*scale:90*scale].sum()>1000,'Missing full-surface effect'
    blocks=crop.reshape(224,scale,256,scale,3)
    smooth_blocks=np.any(blocks!=blocks[:,0:1,:,0:1,:],axis=(1,3,4))
    assert smooth_blocks[32:].sum()>100,'Blur layer is quantized to original pixel blocks'
    report['modes'][name]={'reference_max_channel_error':int(error.max()),
      'reference_mean_channel_error':float(error.mean()),'gui_unchanged':True,
      'changed_display_pixels':int(changed.sum()),'smooth_overlay_blocks':int(smooth_blocks.sum())}
print(json.dumps(report,indent=2))
(root/'Docs/pixel-verification.json').write_text(json.dumps(report,indent=2)+'\n')
