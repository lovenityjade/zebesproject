#include "sm_scavenger.h"
#include "sm_relic.h"
#include "sm_objectives.h"
#include "sm_objective_events.h"
#include "sm_seed_rules.h"
#include "sm_varia_ui.h"
#include "sm_map_exploration.h"
#include "sm_seed.h"
#include "sm_tracker.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include "sm_rtl.h"
#include "sm_cpu_infra.h"
#include <string.h>
#include <stdio.h>
#include "sm_ui_rom_assets.h"
#include "sm_varia_assets.inc"
void sm_varia_ui_load_assets(void){varia_gfx_load();}

/* Preferences are independent of item-placement identity. Always gated by the
 * selected native seed, including when a mixed A/B/C bank changes modes. */
static unsigned options=15;
static uint8_t counted[100];
static int has_counted;
void sm_varia_ui_counted_configure(const uint8_t *counts,int count){
  has_counted=counts && count==100;
  if(has_counted)memcpy(counted,counts,100);
}
static int region,remaining,notification,timer,seen_bosses,primed,seen_relics;
void sm_varia_ui_configure(unsigned mask){options=mask&15;}
static unsigned effective_options(void){return options & ~( (sm_seed_rule(SM_SEED_ORIGINAL_HUD)?SM_VARIA_HUD:0) | (sm_seed_rule(SM_SEED_ORIGINAL_RESERVES)?SM_VARIA_RESERVES:0));}
int sm_varia_ui_active(unsigned flag){return sm_seed_active() && (effective_options()&flag);}
void sm_varia_ui_reset(void){region=remaining=notification=timer=seen_bosses=primed=seen_relics=0;}
int sm_varia_ui_state(int field){
  switch(field){case 0:return sm_seed_active()?effective_options():0;case 1:return region;
    case 2:return remaining;case 3:return notification;case 4:return timer;
    case 5:return !samus_reserve_health?0:samus_reserve_health>=samus_max_reserve_health?2:1;
    default:return 0;}
}
int sm_varia_region(void){return sm_map_exploration_region();}
void sm_varia_ui_frame(void){
  const int scav_prompt=sm_scavenger_state(4);
  sm_scavenger_frame(sm_varia_ui_active(SM_VARIA_HUD));
  if(!sm_seed_active() || (game_state!=8 && game_state!=15 && game_state!=27))return;
  region=sm_varia_region();
  remaining=0;for(int i=0;i<100;i++)remaining+=(!has_counted || counted[i]) && varia_locations[i][0]==region && !sm_seed_location_collected(i);
  if(sm_relic_required()){
    int count=sm_relic_count();
    if(!primed){seen_relics=count;primed=1;}
    if(game_state==15){timer=notification=0;seen_relics=count;return;}
    if(count>seen_relics){notification=count>=sm_relic_required()?7:6;timer=240;seen_relics=count;}
    else if(timer && !--timer)notification=0;
    return;
  }
  if(sm_objectives_state(0)){
    /* Source check_objectives: only acknowledge a displayed notification
     * when its five seconds end or the player opens native pause. Hidden
     * goals reveal only their list number. Completion and notification bits
     * belong to the slot's original SRAM, never to inferred inventory. */
    if(!sm_varia_ui_active(SM_VARIA_HUD)){timer=notification=0;return;}
    if(scav_prompt)return;
    if(timer){
      if(game_state==15 || !--timer){
        if(notification==5)sm_objective_event_mark(128);
        else if(notification>=8)sm_objective_event_mark(163+2*(notification-8));
        timer=notification=0;
      }
      return;
    }
    if(game_state==15)return;
    int index=sm_objectives_check_index();
    if(index>=0 && sm_objective_event(162+2*index) && !sm_objective_event(163+2*index))notification=8+index;
    else if(sm_objective_event(10) && !sm_objective_event(128))notification=5;
    if(notification)timer=300;
    return;
  }
  int bosses=!!(boss_bits_for_area[1]&1)|!!(boss_bits_for_area[3]&1)<<1|
    !!(boss_bits_for_area[4]&1)<<2|!!(boss_bits_for_area[2]&1)<<3;
  if(!primed){seen_bosses=bosses|(bosses==15?16:0);primed=1;}
  if(game_state==15){timer=0;notification=0;seen_bosses=bosses|(seen_bosses&16);return;}
  if(timer){if(!--timer)notification=0;return;}
  int newly=bosses&~seen_bosses;
  if(newly){
    for(int i=0;i<4;i++)if(newly&(1<<i)){seen_bosses|=1<<i;notification=i+1;timer=300;break;}
  }else if(bosses==15 && !(seen_bosses&16)){seen_bosses|=16;notification=5;timer=300;}
}
/* Nodever2/VARIA reserve semantics: consume only energy actually transferred. */
int sm_varia_reserve_transfer(void){
  samus_periodic_subdamage=0;samus_periodic_damage=0;
  if(samus_invincibility_timer<65535)++samus_invincibility_timer;
  if(!(nmi_frame_counter_word&7))QueueSfx3_Max3(0x2d);
  if(!samus_reserve_health || samus_health>=samus_max_health)return 1;
  ++samus_health;--samus_reserve_health;
  return !samus_reserve_health || samus_health>=samus_max_health;
}
int sm_varia_reserve_manual(void){
  if(!sm_varia_ui_active(SM_VARIA_RESERVES))return 0;
  if(pausemenu_reserve_tank_delay_ctr && (joypad1_newkeys&kButton_A)){
    pausemenu_reserve_tank_delay_ctr=0;pausemenu_equipment_category_item=0;
    EquipmentScreenEnergyArrowGlow_Off();return 1;
  }
  if(!pausemenu_reserve_tank_delay_ctr){
    if(!(joypad1_newkeys&kButton_A))return 1;
    pausemenu_reserve_tank_delay_ctr=(samus_reserve_health+7)&~7;
  }
  if(pausemenu_reserve_tank_delay_ctr && !(--pausemenu_reserve_tank_delay_ctr&7))QueueSfx3_Max6(0x2d);
  if(samus_reserve_health && samus_health<samus_max_health){++samus_health;--samus_reserve_health;}
  if(!samus_reserve_health || samus_health>=samus_max_health){
    pausemenu_reserve_tank_delay_ctr=0;pausemenu_equipment_category_item=0;EquipmentScreenEnergyArrowGlow_Off();
  }
  return 1;
}
extern uint8_t sm_wide_hud[];
extern uint16_t sm_hud_palette[32];
static uint8_t *native_target;
static void pixel(uint8_t *out,int w,int x,int y,uint16_t c){
  if(x<0 || x>=w || y<0 || y>=224)return;
  int brightness=g_snes->ppu->forcedBlank?0:g_snes->ppu->brightness;
  uint8_t *p=out+(y*w+x)*4;
  p[0]=((c>>10)&31)*255/31*brightness/15;p[1]=((c>>5)&31)*255/31*brightness/15;
  p[2]=(c&31)*255/31*brightness/15;p[3]=out==native_target || c?255:0;
}
static void clear(uint8_t *out,int w,int x,int y,int sw,int sh){
  for(int iy=y;iy<y+sh;iy++)for(int ix=x;ix<x+sw;ix++)pixel(out,w,ix,iy,0);
}
static void tile(uint8_t *out,int w,int x,int y,unsigned id,int palette){
  const uint8_t *gfx=varia_gfx+(id&255)*16;
  for(int dy=0;dy<8;dy++)for(int dx=0;dx<8;dx++){
    int row=(id&0x8000)?7-dy:dy,bit=(id&0x4000)?dx:7-dx;
    int ci=((gfx[row*2]>>bit)&1)|(((gfx[row*2+1]>>bit)&1)<<1);
    uint16_t c=sm_hud_palette[palette*4+ci];
    if(palette==7){static const uint16_t colors[]={0,0x02df,0x48fb,0x7fff};c=colors[ci];}
    pixel(out,w,x+dx,y+dy,ci?c:0);
  }
}
static void text(uint8_t *out,int w,int x,int y,const char *s){
  for(;*s;s++,x+=8){unsigned id=varia_font[(unsigned char)*s&127];tile(out,w,x,y,id,(id>>10)&7);}
}
static void digits(uint8_t *out,int w,int x,int y,int number,int count,int maximum,int palette){
  static const uint8_t high[]={0x45,0x3c,0x3d,0x3e,0x3f,0x40,0x41,0x42,0x43,0x44};
  static const uint8_t low[]={9,0,1,2,3,4,5,6,7,8};
  if(number>999)number=999;
  for(int i=count-1;i>=0;i--){tile(out,w,x+i*8,y,maximum?high[number%10]:low[number%10],palette);number/=10;}
}
static void draw(uint8_t *out,int w){
  const int shift=w==400?72:0;
  if(sm_varia_ui_active(SM_VARIA_AMMO)){
    const int caps[]={samus_max_missiles,samus_max_super_missiles,samus_max_power_bombs};
    const int ammo[]={samus_missiles,samus_super_missiles,samus_power_bombs};
    const int xpos[]={80,104,128};
    for(int i=0;i<3;i++){
      int x=xpos[i]+shift;clear(out,w,x,8,24,24);if(!caps[i])continue;
      int pal=hud_item_index==i+1?4:5;
      int count=i==0 || caps[i]>=100?3:2;
      digits(out,w,x+(3-count)*8,8,caps[i],count,1,pal);
      if(!i)for(int n=0;n<3;n++)tile(out,w,x+n*8,16,0x49+n,pal);
      else for(int n=0;n<2;n++)tile(out,w,x+8+n*8,16,0x34+(i-1)*2+n,pal);
      count=i==0 || ammo[i]>=100?3:2;
      digits(out,w,x+(3-count)*8,24,ammo[i],count,0,
        samus_auto_cancel_hud_item_index==i+1 && (nmi_frame_counter_word&0x10)?4:3);
    }
  }
  const int hud=sm_varia_ui_active(SM_VARIA_HUD);
  if(hud){
    static const char *names[]={" Ceres "," Crater","Gr Brin","Red Bri"," W Ship"," Kraid ","Up Norf"," Croc  ","Lo Norf","W Marid","E Marid","Tourian"};
    clear(out,w,0,8,80,16);
    const char *scav=sm_scavenger_label();
    if(scav && (sm_scavenger_state(4) || !notification)){
      text(out,w,0,8,scav);
      int rank=sm_scavenger_state(5);
      if(rank && !sm_scavenger_state(4)){
        tile(out,w,64,8,0x10+(rank>=10?rank/10:rank),3);
        if(rank>=10)tile(out,w,72,8,0x10+rank%10,3);
      }
    }else if(notification>=8){
      const int number=notification-7;
      text(out,w,0,8,"Obj OK! ");
      if(number>=10)tile(out,w,64,8,0x10+number/10,3);
      tile(out,w,number>=10?72:64,8,0x10+number%10,3);
    }
    else if(notification>=6){
      if(notification==7)text(out,w,0,8,"Ship OK!");
      else {text(out,w,0,8,"Relic ");tile(out,w,48,8,0x10+sm_relic_count()/10,3);tile(out,w,56,8,0x10+sm_relic_count()%10,3);}
    }
    else if(notification){text(out,w,0,8,notification==5?" Objs OK! ":"Obj OK! ");if(notification<5)tile(out,w,64,8,0x10+notification,3);}
    else {text(out,w,0,8,names[region]);tile(out,w,64,8,0x10+remaining/10,3);tile(out,w,72,8,0x10+remaining%10,3);}
    for(int i=0;i<7;i++){
      int threshold=(i+1)*100,id=samus_max_health<threshold?0xf:samus_health>=threshold+700?0x31:samus_health>=threshold?0x4f:0x30;
      int pal=samus_health>=threshold+700?3:samus_health>=threshold?7:5;tile(out,w,8+i*8,16,id,pal);
    }
  }
  if(sm_varia_ui_active(SM_VARIA_RESERVES) && reserve_health_mode==1 && samus_max_reserve_health){
    int full=samus_reserve_health>=samus_max_reserve_health;
    const int tiles[]={full?0xca:0x33,full?0xd9:0x46,full?0xda:0x47,full?0xdb:0x48};
    int pal=full?7:samus_reserve_health?7:3;
    if(!hud){tile(out,w,64,8,tiles[0],pal);tile(out,w,72,8,tiles[1],pal);}
    tile(out,w,64,16,tiles[2],pal);tile(out,w,72,16,tiles[3],pal);
    tile(out,w,64,24,tiles[0]|0x8000,pal);tile(out,w,72,24,tiles[1]|0x8000,pal);
  }
}
void sm_varia_ui_render(uint8_t *native_pixels){
  if(!sm_seed_active() || (game_state!=8 && game_state!=15 && game_state!=27))return;
  native_target=native_pixels;draw(native_pixels,256);draw((uint8_t*)sm_ui_overlay(),256);draw(sm_wide_hud,400);
}
