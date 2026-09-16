#include "../Native/sm_relief.h"
#include <assert.h>
#include <stdio.h>
int main(void) {
  static uint8_t source[256*240*4],before[sizeof source],out[sizeof source];
  static uint8_t layers[sizeof source],far[256*240];
  for(int y=0;y<240;++y)for(int x=0;x<256;++x) {
    int i=y*256+x,solid=x>=80 && x<176 && y>=64 && y<192;
    layers[i*4]=solid?0:1;layers[i*4+3]=255;
    far[i]=solid?0:255;
    for(int c=0;c<4;++c)source[i*4+c]=c==3?173:120;
  }
  // Moving OAM over the middle of the wall must not bevel the wall around it.
  for(int y=96;y<112;++y)for(int x=120;x<128;++x)layers[(y*256+x)*4]=4;
  memcpy(before,source,sizeof source);
  sm_compose_relief(source,layers,far,out);
  assert(!memcmp(before,source,sizeof source));
  int lighter=0,darker=0;
  for(int y=0;y<240;++y)for(int x=0;x<256;++x) {
    int i=(y*256+x)*4;
    if(!sm_relief_structure(layers,far,x,y))assert(!memcmp(source+i,out+i,4));
    assert(source[i+3]==out[i+3]);
    lighter+=out[i]>source[i];darker+=out[i]<source[i];
  }
  assert(lighter>100 && darker>100);
  assert(out[(64*256+100)*4]>120); // Upper edge illuminated.
  assert(out[(191*256+100)*4]<120); // Lower edge recessed.
  assert(out[(100*256+119)*4]==120 && out[(100*256+128)*4]==120);
  assert(out[(140*256+128)*4]==120); // No tile-grid or palette emboss.
  printf("PASS relief: %d lit, %d shaded; source, HUD, sprites, far planes and alpha preserved.\n",lighter,darker);
}
