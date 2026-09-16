#pragma once
#include <stdint.h>
#include <string.h>

/* A pronounced bevel on the visible BG1/far-plane boundary, before the Gaussian
 * overlay. Layer identity supplies geometry; palette brightness is not height.
 * No displacement, sprite modification, or silhouette expansion. */
static inline int sm_relief_structure(const uint8_t *layers,const uint8_t *far,int x,int y,int width,int top) {
  if(x<0 || x>=width || y<top || y>=224)return 0;
  int i=y*width+x;
  return layers[4*i+3] && layers[4*i]==0 && !far[i];
}
static inline float sm_relief_edge(const uint8_t *layers,const uint8_t *far,
                                   int x,int y,int dx,int dy,int width,int top) {
  for(int d=1;d<=6;++d) {
    int sx=x+dx*d,sy=y+dy*d;
    if(sx<0 || sx>=width || sy<top || sy>=224)return 0;
    int i=sy*width+sx;
    if(layers[4*i+3] && far[i])return (7-d)/6.f;
    // OAM never creates a bevel cut into the wall behind a moving sprite.
    if(!sm_relief_structure(layers,far,sx,sy,width,top))return 0;
  }
  return 0;
}
static inline void sm_compose_relief_size(const uint8_t *source,const uint8_t *layers,
                                     const uint8_t *far,uint8_t *output,int width,int top) {
  memcpy(output,source,width*240*4);
  for(int y=top;y<224;++y)for(int x=0;x<width;++x) {
    if(!sm_relief_structure(layers,far,x,y,width,top))continue;
    float topEdge=sm_relief_edge(layers,far,x,y,0,-1,width,top);
    float left=sm_relief_edge(layers,far,x,y,-1,0,width,top);
    float bottom=sm_relief_edge(layers,far,x,y,0,1,width,top);
    float right=sm_relief_edge(layers,far,x,y,1,0,width,top);
    float lit=topEdge>left?topEdge:left,shade=bottom>right?bottom:right;
    float gain=1.f+.40f*lit-.50f*shade;
    int i=(y*width+x)*4;
    for(int c=0;c<3;++c) {
      int value=(int)(source[i+c]*gain+.5f);
      output[i+c]=(uint8_t)(value>255?255:value);
    }
  }
}

static inline void sm_compose_relief(const uint8_t *s,const uint8_t *l,const uint8_t *f,uint8_t *o) {sm_compose_relief_size(s,l,f,o,256,32);}
