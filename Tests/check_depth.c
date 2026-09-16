#include "../Native/sm_depth.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
  static uint8_t source[256*240*4],before[256*240*4],layers[256*240*4],far[256*240],out[256*240*4];
  for(int y=0;y<240;++y)for(int x=0;x<256;++x) {
    int i=y*256+x;
    for(int c=0;c<4;++c)source[i*4+c]=c==3?255:180;
    layers[i*4]=(x<64?0:1);layers[i*4+3]=255;
    far[i]=y>=32 && y<224 && x>=64 ? 255:0;
  }
  // Flat sprite in front of the far plane.
  for(int y=90;y<110;++y)for(int x=140;x<148;++x) {
    int i=y*256+x;layers[i*4]=4;far[i]=0;source[i*4+2]=255;
  }
  memcpy(before,source,sizeof(source));
  sm_compose_depth(source,layers,far,out);
  assert(!memcmp(source,before,sizeof(source)));
  int changed=0;
  for(int i=0;i<256*240;++i) {
    if(!far[i])assert(!memcmp(source+i*4,out+i*4,4));
    else {assert(out[i*4]<source[i*4]);++changed;}
    assert(out[i*4+3]==source[i*4+3]);
  }
  assert(out[(70*256+65)*4]<out[(70*256+90)*4]); // Contact shadow by frame.
  assert(out[(108*256+149)*4]<out[(108*256+180)*4]); // Sprite casts only onto background.
  for(int i=0;i<256*240;++i)if(far[i])far[i]=64;
  far[70*256+90]=128;far[70*256+91]=192;
  sm_compose_depth(source,layers,far,out);
  assert(out[(70*256+65)*4]==out[(70*256+180)*4]);
  assert(out[(108*256+149)*4]==out[(108*256+180)*4]);
  assert(out[(70*256+90)*4]>out[(70*256+91)*4]);
  printf("PASS depth: %d far texels shaded; source, foreground, sprite, HUD and alpha intact; contact shadows present.\n",changed);
}
