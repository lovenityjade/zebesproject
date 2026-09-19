#include "sm_locale.h"
#include "sm_achievements.h"
#include "sm_notifications.h"
#include "sm_map_browser.h"
#include "sm_scene.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
static uint64_t pending;
static int current=-1,remaining;
static uint8_t icons[20][32*32*4];
extern uint8_t sm_wide_hud[];
void sm_notifications_reset(void){pending=0;current=-1;remaining=0;sm_achievements_reset();}
void sm_achievement_notify(unsigned bits){sm_achievement_notify64(bits&31);}
void sm_achievement_notify64(uint64_t bits){pending|=bits&SM_ACHIEVEMENT_MASK;}
int sm_achievement_set_icon(int icon,const uint8_t *bgra,int w,int h){
 if(icon<0||icon>=20||!bgra||w<1||h<1||w>2048||h>2048)return 0;
 for(int y=0;y<32;y++)for(int x=0;x<32;x++)memcpy(icons[icon]+(y*32+x)*4,bgra+((y*h/32)*w+x*w/32)*4,4);
 return 1;
}
static void draw(uint8_t *out,int width){
 const int x=(width-240)/2,y=176;
 for(int j=0;j<44;j++)for(int i=0;i<240;i++){
  uint8_t *p=out+((y+j)*width+x+i)*4;int edge=i==0||i==239||j==0||j==43;
  p[0]=edge?130:12;p[1]=edge?105:8;p[2]=edge?45:4;p[3]=255;
 }
 const SmAchievementDefinition *d=&sm_achievement_catalog[current];
 for(int j=0;j<32;j++)for(int i=0;i<32;i++){
  const uint8_t *src=icons[d->icon]+(j*32+i)*4;uint8_t *dst=out+((y+6+j)*width+x+6+i)*4;
  for(int k=0;k<3;k++)dst[k]=(src[k]*src[3]+dst[k]*(255-src[3]))/255;
 }
 sm_native_map_text(out,width,x+44,y+6,"ACHIEVEMENT UNLOCKED",0x84cfff);
 const char *name=sm_locale_get()?d->name_fr:d->name;
 char label[128];const char *p=name;int count=0;
 while(*p&&count<23){sm_locale_next(&p);count++;}
 size_t n=(size_t)(p-name);memcpy(label,name,n);label[n]=0;
 sm_native_map_text(out,width,x+44,y+25,label,0xffffff);
}
void sm_notifications_frame(uint8_t *pixels){
  if((game_state!=8 && game_state!=39) || sm_message_active())return;
  if(current<0 && pending){
    for(current=0;current<SM_ACHIEVEMENT_COUNT && !(pending&(UINT64_C(1)<<current));current++);
    pending&=~(UINT64_C(1)<<current);remaining=180;QueueSfx1_Max6(0x37);
  }
  if(current<0)return;
  draw(pixels,256);draw((uint8_t*)sm_ui_overlay(),256);draw(sm_wide_hud,400);
  if(--remaining==0)current=-1;
}
