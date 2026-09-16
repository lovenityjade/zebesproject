#pragma once
#include <stdint.h>
#include <string.h>
/* Presentation-only planes. Far surfaces receive frame/sprite contact shadows.
 * The input framebuffer, collision geometry and foreground texels stay intact. */
static inline int sm_depth_solid(const uint8_t *layers,const uint8_t *far,int x,int y,int width,int top) {
  if(x<0||x>=width||y<top||y>=224)return 0;
  int i=y*width+x,layer=layers[i*4];
  return layers[i*4+3] && !far[i] && (layer==0||layer==4||layer==6);
}
static inline void sm_compose_depth_size(const uint8_t *source,const uint8_t *layers,
                                    const uint8_t *far,uint8_t *output,int width,int top) {
  memcpy(output,source,width*240*4);
  const int weights[5]={1,4,6,4,1};
  for(int y=top;y<224;++y)for(int x=0;x<width;++x) {
    int i=y*width+x;if(!far[i])continue;
    if(far[i]<255) {
      // Distant outdoor planes never receive a ship/sprite drop shadow.
      float shade=far[i]<100?.98f:(far[i]<160?.96f:.94f);
      for(int c=0;c<3;++c)output[i*4+c]=(uint8_t)(source[i*4+c]*shade+.5f);
      continue;
    }
    float contact=0;
    for(int d=1;d<=5;++d) {
      if(sm_depth_solid(layers,far,x-d,y,width,top)||sm_depth_solid(layers,far,x+d,y,width,top)||
         sm_depth_solid(layers,far,x,y-d,width,top)||sm_depth_solid(layers,far,x,y+d,width,top)) {
        contact=(6-d)/5.f;break;
      }
    }
    int shadow=0;
    for(int dy=-2;dy<=2;++dy)for(int dx=-2;dx<=2;++dx)
      shadow+=weights[dy+2]*weights[dx+2]*sm_depth_solid(layers,far,x-2+dx,y-3+dy,width,top);
    float shade=.91f-.16f*contact-.16f*(shadow/256.f);
    for(int c=0;c<3;++c)output[i*4+c]=(uint8_t)(source[i*4+c]*shade+.5f);
  }
}

static inline void sm_compose_depth(const uint8_t *s,const uint8_t *l,const uint8_t *f,uint8_t *o) {sm_compose_depth_size(s,l,f,o,256,32);}
