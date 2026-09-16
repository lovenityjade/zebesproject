#include "ida_types.h"
#include "variables.h"
#include "sm_sprite_view.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
uint8 g_ram[0x20000];
static void check(int base,int offset,int expected){
  uint32 words=48u|((unsigned)(expected&255)<<8)|0x12340000u;
  memcpy(oam_ent,&words,4);sm_sprite_view_record(0,48,0,(uint16)base,(uint8)offset);
  int y=9999;assert(sm_sprite_view_y(0,words,&y)&&y==expected);
  assert(!sm_sprite_view_y(0,words^0x10000,&y));
}
int main(void){
  sm_sprite_view_reset();
  check(260,0,260);check(280,-16,264);check(230,16,246);
  check(0,-16,-16);check(-12,16,4);check(512,0,512);
  sm_sprite_view_reset();int y;assert(!sm_sprite_view_y(0,0,&y));
  puts("SPRITE_SIGNED_Y_BOUNDARIES_AND_STALE_OAM_PASS");
}
