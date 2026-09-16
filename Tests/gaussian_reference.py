"""Independent display-space Gaussian + Photoshop blend reference for captures."""
import numpy as np
def compose(raw,scale,opacity,exposure,mode='Lighten',hud_top=32):
    width=raw.shape[1]
    px=(np.arange(width*scale)+.5)/scale;py=(np.arange(224*scale)+.5)/scale
    cx=np.floor(px).astype(int);cy=np.floor(py).astype(int)
    base=raw.repeat(scale,0).repeat(scale,1)/255.
    blur=np.zeros(base.shape,np.float64);total=np.zeros(base.shape[:2],np.float64)
    for dy in range(-4,5):
        wy=np.exp(-((cy+dy+.5-py)**2)/(2*1.25**2))
        iy=np.clip(cy+dy,hud_top,223)
        for dx in range(-4,5):
            wx=np.exp(-((cx+dx+.5-px)**2)/(2*1.25**2))
            weight=wy[:,None]*wx[None,:]
            sample=raw[iy[:,None],np.clip(cx+dx,0,width-1)[None,:]]/255.
            blur+=sample*weight[:,:,None];total+=weight
    blur=np.clip(blur/total[:,:,None]*exposure,0,1);base=np.clip(base*exposure,0,1)
    mixed=np.maximum(base,blur) if mode=='Lighten' else base*blur
    result=np.rint((base+opacity*(mixed-base))*255).clip(0,255).astype(np.uint8)
    result[:hud_top*scale]=raw[:hud_top].repeat(scale,0).repeat(scale,1)
    return result
