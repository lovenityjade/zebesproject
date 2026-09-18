#include "sm_locale.h"
#include "sm_notifications.h"
#include "sm_map_browser.h"
#include "sm_scene.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
static unsigned pending;
static int current=-1,remaining;
static const char *names[]={"NEW UPGRADE","TEN ENEMIES DEFEATED","POWER BOMB","GRAPPLE BEAM","SCREW ATTACK"};
extern uint8_t sm_wide_hud[];
void sm_notifications_reset(void){pending=0;current=-1;remaining=0;}
void sm_achievement_notify(unsigned bits){pending|=bits&31;}
static void draw(uint8_t *out,int width){
  int x=(width-208)/2,y=184;
  for(int j=0;j<32;j++)for(int i=0;i<208;i++){
    uint8_t *p=out+((y+j)*width+x+i)*4;
    int edge=i==0||i==207||j==0||j==31;
    p[0]=edge?130:12;p[1]=edge?105:8;p[2]=edge?45:4;p[3]=255;
  }
  sm_native_map_text(out,width,x+20,y+5,"ACHIEVEMENT UNLOCKED",0x84cfff);
  const char *name=sm_locale_text(names[current]);sm_native_map_text(out,width,(width-sm_locale_length(name)*8)/2,y+18,name,0xffffff);
}
void sm_notifications_frame(uint8_t *pixels){
  if(game_state!=8 || sm_message_active())return;
  if(current<0 && pending){
    for(current=0;current<5 && !(pending&(1u<<current));current++);
    pending&=~(1u<<current);remaining=180;QueueSfx1_Max6(0x37);
  }
  if(current<0)return;
  draw(pixels,256);draw((uint8_t*)sm_ui_overlay(),256);draw(sm_wide_hud,400);
  if(--remaining==0)current=-1;
}
