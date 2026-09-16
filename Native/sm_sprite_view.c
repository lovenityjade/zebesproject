#include "sm_sprite_view.h"
#include "ida_types.h"
#include "variables.h"
#include <string.h>
static struct {uint32_t words;int x,y,valid,gui;} sprites[128];
static int gui;
void sm_sprite_view_reset(void) {memset(sprites,0,sizeof(sprites));gui=0;}
void sm_sprite_view_set_gui(int value) {gui=!!value;}
void sm_sprite_view_record(int index,uint16_t base,uint16_t offset,uint16_t base_y,uint8_t offset_y) {
  if(index<0 || index>=512)return;
  int i=index/4,dx=offset&511;if(dx&256)dx-=512;
  // The hardware truncates X to nine bits. Keep its full signed coordinate
  // beside the simulation so distant sprites cannot wrap onto the other edge.
  memcpy(&sprites[i].words,(uint8_t*)oam_ent+index,4);
  sprites[i].y=(int16_t)base_y+(int8_t)offset_y;
  sprites[i].x=(int16_t)base+dx;sprites[i].valid=1;sprites[i].gui=gui;
}
int sm_sprite_view_gui(int index,uint32_t words) {
  int i=index/2;
  return i>=0 && i<128 && sprites[i].valid && sprites[i].words==words && sprites[i].gui;
}
int sm_sprite_view_x(int index,uint32_t words,int *x) {
  int i=index/2;
  if(i<0 || i>=128 || !sprites[i].valid || sprites[i].words!=words)return 0;
  *x=sprites[i].x;return 1;
}

int sm_sprite_view_y(int index,uint32_t words,int *y) {
  int i=index/2;
  if(i<0 || i>=128 || !sprites[i].valid || sprites[i].words!=words ||
     (sprites[i].y&255)!=((words>>8)&255))return 0;
  *y=sprites[i].y;return 1;
}
