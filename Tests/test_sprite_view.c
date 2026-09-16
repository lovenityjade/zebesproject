/* Independent coordinate cases that alias in SNES OAM but not a wide camera. */
#include "sm_sprite_view.h"
#include "ida_types.h"
#include "variables.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
uint8 g_ram[0x20000];
int main(void) {
  int xs[]={-600,-212,-73,-72,-1,0,255,256,300,327,328,512,700};
  int ds[]={-32,-8,0,16},count=0;
  for(int i=0;i<13;i++)for(int j=0;j<4;j++)for(int large=0;large<2;large++) {
    int expected=xs[i]+ds[j],actual=0;
    oam_ent[0].xcoord=expected;oam_ent[0].ycoord=100;
    oam_ent[0].charnum=7;oam_ent[0].flags=0;
    uint32_t words;memcpy(&words,oam_ent,4);
    sm_sprite_view_record(0,(uint16_t)xs[i],(ds[j]&511)|(large?0x8000:0));
    assert(sm_sprite_view_x(0,words,&actual) && actual==expected);
    assert(!sm_sprite_view_gui(0,words));
    assert(!sm_sprite_view_x(0,words^0x10000,&actual));
    sm_sprite_view_set_gui(1);sm_sprite_view_record(0,(uint16_t)xs[i],ds[j]&511);
    assert(sm_sprite_view_gui(0,words));assert(!sm_sprite_view_gui(0,words^0x10000));
    sm_sprite_view_set_gui(0);
    ++count;
  }
  sm_sprite_view_reset();int actual;
  assert(!sm_sprite_view_x(0,0,&actual));
  printf("SM_SPRITE_COORDINATES_PASS cases=%d staleOamRejected=true\n",count);
}
