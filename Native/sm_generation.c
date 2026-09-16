#include "sm_start.h"
#include "sm_relic.h"
#include "sm_generation.h"
#include "sm_runs.h"
#include "sm_route.h"
#include "sm_travel.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
#include "sm_rtl.h"
#include "sm_tracker.h"
#include "sm_run_stats.h"
#include "sm_bridge.h"
static int mode,state,requested,save_error;
static int display_mode,difficulty;
typedef struct {int mode,ready;SmSeedItem items[100];char hash[65];} Slot;
static Slot slots[3];
static int managed,current=-1;
static SmSlotAction action_callback;
static void *action_context;
void sm_slots_reset(void){sm_start_reset();sm_relic_reset();sm_travel_reset();managed=0;current=-1;action_callback=0;action_context=0;memset(slots,0,sizeof(slots));}
void sm_slots_enable(SmSlotAction callback,void *context){managed=1;action_callback=callback;action_context=context;}
int sm_slots_managed(void){return managed;}
int sm_slots_current(void){return managed?current:-1;}
int sm_slots_editable(void){return managed && current>=0 && game_state==2 && game_options_screen_index==3 && state!=1 && !(nonempty_save_slots&(1<<current)) && !(slots[current].mode && slots[current].ready);}
int sm_slots_set(int slot,int randomized,int ready,const SmSeedItem *items,int count,const char *hash){
  if(!managed || slot<0 || slot>2 || (randomized && ready && (!items || count!=100 || !hash || strlen(hash)!=64)))return 0;
  if(randomized && ready && !sm_seed_validate_plan(items,count,hash))return 0;
  Slot next={0};next.mode=!!randomized;next.ready=!!ready;
  if(randomized && ready){memcpy(next.items,items,sizeof(next.items));memcpy(next.hash,hash,65);}
  slots[slot]=next;return 1;
}
int sm_slots_select(int slot){
  if(!managed)return 1;
  if(slot<0 || slot>2)return 0;
  Slot *s=&slots[slot];
  int previous=sm_start_active_slot();
  if(!sm_start_activate(slot,s->mode&&s->ready))return 0;
  if(!sm_seed_select_plan(s->mode&&s->ready?s->items:0,100,s->hash)){sm_start_activate(previous,previous>=0);return 0;}
  current=slot;sm_generation_configure(s->mode,s->ready);sm_tracker_new_session();return 1;
}
int sm_slots_action(int action,int slot,int other){
  if(!managed)return 1;
  if(slot<0 || slot>2 || !action_callback)return 0;
  if(action==2 && (other<0 || other>2 || other==slot))return 0;
  char source_seed[65],target_seed[65];
  memcpy(source_seed,slots[slot].hash,65);
  memcpy(target_seed,slots[action==2?other:slot].hash,65);
  int ok=action_callback(action,slot,other,action_context);
  if(ok){sm_route_slot_action(action,slot,other,source_seed,target_seed);sm_stats_slot_action(action,slot,other);sm_relic_escape_slot_action(action,slot,other);sm_run_slot_action(action,slot,other);}
  return ok;
}
int sm_slots_commit(const SmSeedItem *items,int count,const char *hash){
  return current>=0 && sm_slots_set(current,1,1,items,count,hash);
}
void sm_slots_badges(void){/* Emblems are drawn in sm_generation_render. */}
int sm_slots_mode_badges_active(void){return managed && game_state==4 && (menu_index==2 || menu_index==3 || menu_index==4 || menu_index==5 || menu_index==16 || menu_index==17);}
void sm_generation_configure(int randomized,int ready) {
  mode=!!randomized;state=ready?2:0;requested=save_error=0;display_mode=mode?SM_MODE_RANDOMIZER:SM_MODE_VANILLA;
}
int sm_generation_state(void){return mode?state:-1;}
int sm_generation_take_request(void){int r=requested;requested=0;return r;}
void sm_generation_fail(int disk_error){if(mode && state==1){state=3;save_error=!!disk_error;}}
int sm_generation_can_start(void){return (display_mode==SM_MODE_VANILLA && !mode) || (display_mode==SM_MODE_RANDOMIZER && mode && state==2);}
int sm_generation_is_working(void){return mode && state==1 && game_state==2 && game_options_screen_index==3 && !loading_game_state;}
void sm_generation_complete(void){state=2;requested=0;menu_option_index=0;}
int sm_generation_menu(int field){
  switch(field){case 0:return display_mode;case 1:return difficulty;case 2:return menu_option_index;case 3:return sm_generation_can_start();default:return -1;}
}
void sm_generation_enter(void){display_mode=mode?SM_MODE_RANDOMIZER:SM_MODE_VANILLA;difficulty=0;menu_option_index=1;}
static int row_y(int row){return row==1?48:row==2?120:row==3?144:row==4?144:184;}
int sm_generation_cursor(uint16_t object){
  if(game_state!=2 || (game_options_screen_index!=2 && game_options_screen_index!=3))return 0;
  int i=object>>1;eproj_y_pos[i+13]=menu_option_index==1?384:24;eproj_x_vel[i+3]=row_y(menu_option_index)+8;return 1;
}
static void change_mode(int direction){
  if((managed && !sm_slots_editable()) || (!managed && (sm_seed_active() || loading_game_state))){QueueSfx1_Max6(0x3d);return;}
  int next=(display_mode+direction+4)%4;
  // Unavailable modes are previews only. Never replace a real slot with one.
  if(next==SM_MODE_VANILLA || next==SM_MODE_RANDOMIZER){
    int randomized=next==SM_MODE_RANDOMIZER;
    if(randomized!=mode){
      if(managed){if(!sm_slots_action(0,current,randomized) || !sm_slots_select(current)){QueueSfx1_Max6(0x3d);return;}}
      else sm_generation_configure(randomized,0);
    }
  }
  display_mode=next;QueueSfx1_Max6(0x37);
}
int sm_generation_input(void) {
  if(state==1)return 1;
  int rows[4]={1,0,0,0},count=2;
  if(display_mode==SM_MODE_RANDOMIZER){rows[1]=2;rows[2]=4;rows[3]=0;count=4;}
  else if(display_mode==SM_MODE_BOSS_RUSH){rows[1]=3;rows[2]=0;count=3;}
  int index=0;for(int i=0;i<count;i++)if(rows[i]==menu_option_index)index=i;
  if(joypad1_newkeys&kButton_Up){index=(index+count-1)%count;QueueSfx1_Max6(0x37);}
  else if(joypad1_newkeys&kButton_Down){index=(index+1)%count;QueueSfx1_Max6(0x37);}
  menu_option_index=rows[index];
  if(joypad1_newkeys&kButton_B){game_options_screen_index=11;return 1;}
  int confirm=joypad1_newkeys&(kButton_A|kButton_Start),direction=joypad1_newkeys&kButton_Left?-1:joypad1_newkeys&kButton_Right?1:0;
  if(menu_option_index==1 && (confirm || direction)){change_mode(direction?direction:1);return 1;}
  if(menu_option_index==3 && (confirm || direction)){difficulty=(difficulty+(direction?direction:1)+4)%4;QueueSfx1_Max6(0x37);return 1;}
  if(!confirm)return 1;
  int ok=0;
  if(menu_option_index==0 && sm_generation_can_start()){GameOptionsMenuItemFunc_0();ok=1;}
  else if(menu_option_index==2 && display_mode==SM_MODE_RANDOMIZER && managed && sm_slots_editable())ok=sm_slots_action(1,current,0);
  else if(menu_option_index==4 && display_mode==SM_MODE_RANDOMIZER && mode && state!=2 && !loading_game_state){state=1;requested=1;save_error=0;ok=1;}
  QueueSfx1_Max6(ok?0x38:0x3d);return 1;
}
// Original two-tile menu alphabet. V shares U's upper half: 2E is blank.
static void glyph(char c,uint16_t *top,uint16_t *bottom) {
  static const uint8_t tops[26]={0x0a,0x0b,0x0c,0x0d,0x0e,0x0e,0x0c,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x00,0x0d,0x00,0x0d,0x2b,0x2c,0x2d,0x2d,0x2d,0x40,0x41,0x42};
  static const uint8_t bots[26]={0x1a,0x1b,0x1c,0x1d,0x1e,0x25,0x30,0x31,0x11,0x33,0x34,0x35,0x36,0x37,0x10,0x38,0x39,0x3a,0x3b,0x11,0x3d,0x3e,0x3f,0x50,0x11,0x52};
  *top=c>='A'&&c<='Z'?tops[c-'A']:0x0f;*bottom=c>='A'&&c<='Z'?bots[c-'A']:0x0f;
}
static void text(int row,const char *label,int disabled) {
  int x=(32-(int)strlen(label))/2;
  for(;*label && x<31;x++,label++){
    uint16_t a,b;glyph(*label,&a,&b);uint16_t pal=disabled?0x400:0;
    ram3000.pause_menu_map_tilemap[row*32+x]=a|pal;
    ram3000.pause_menu_map_tilemap[(row+1)*32+x]=b|pal;
  }
}
static uint16_t small_glyph(char c){
  if(c>='A'&&c<='Z')return 0x6a+c-'A';if(c>='0'&&c<='9')return 0x60+c-'0';
  switch(c){case '.':return 0x88;case ',':return 0x89;case '!':return 0x84;case '-':return 0x87;case ':':return 0x8c;default:return 0x0f;}
}
static void small(int row,const char *label,int disabled){
  int x=(32-(int)strlen(label))/2;for(;*label && x<31;x++,label++)if(x>=0)ram3000.pause_menu_map_tilemap[row*32+x]=small_glyph(*label)|(disabled?0x400:0);
}
void sm_generation_draw(void) {
  if(game_options_screen_index!=2 && game_options_screen_index!=3)return;
  // Keep the native frame/header and controller footer; rebuild only the body.
  for(int y=5;y<25;y++)for(int x=1;x<31;x++)ram3000.pause_menu_map_tilemap[y*32+x]=0x0f;
  const char *names[]={"VANILLA MODE","STORY MODE","BOSS RUSH MODE","RANDOMIZER MODE"};
  int future=display_mode==SM_MODE_STORY || display_mode==SM_MODE_BOSS_RUSH;
  text(6,names[display_mode],future);
  uint16_t arrows=menu_option_index==1?0:0x400;
  ram3000.pause_menu_map_tilemap[7*32+2]=0x405c|arrows;ram3000.pause_menu_map_tilemap[7*32+29]=0x5c|arrows;
  if(display_mode==SM_MODE_VANILLA){
    small(10,"THE ORIGINAL ADVENTURE",0);small(12,"EXPLORE ZEBES AS SAMUS ARAN",0);
  }else if(display_mode==SM_MODE_STORY){
    small(9,"VANILLA REIMAGINED WITH",0);small(11,"CINEMATICS, VOICE ACTING",0);small(13,"AND AN EXPANDED STORY",0);small(17,"COMING SOON",1);
  }else if(display_mode==SM_MODE_BOSS_RUSH){
    small(9,"DEFEAT EVERY BOSS AND MINIBOSS",0);small(11,"AS FAST AS POSSIBLE WITH",0);small(13,"MINIMUM ITEMS. ONE CHANCE.",0);
    small(16,"DIFFICULTY",1);
    const char *levels[]={"EASY","MEDIUM","HARD","HARDCORE"};text(18,levels[difficulty],1);
    ram3000.pause_menu_map_tilemap[19*32+5]=0x445c;ram3000.pause_menu_map_tilemap[19*32+26]=0x45c;
    small(21,"COMING SOON",1);
  }else{
    small(9,"ITEMS, ROUTES AND GOALS",0);small(11,"SHUFFLED BY YOUR SETTINGS",0);small(13,"EVERY SEED IS A NEW ADVENTURE",0);
    text(15,"RANDOMIZER OPTIONS",!sm_slots_editable());
    text(18,state==1?"GENERATING":"GENERATE GAME",state==1 || state==2);
    small(21,state==2?"GAME GENERATED":state==1?"GENERATION IN PROGRESS":state==3?(save_error?"SAVE ERROR - OPEN OPTIONS":"CHECK RANDOMIZER OPTIONS"):"GENERATE A SEED TO START",state==1);
  }
  text(23,"START GAME",!sm_generation_can_start());
}

extern uint8_t sm_wide_hud[],sm_wide_pixels[];
static void badge(uint8_t *out,int width,int x,int y,int kind,int disabled){
  if(kind==SM_MODE_STORY){
    uint8_t picture[16*16*4]={0};sm_relic_icon(picture,16,0,0);
    for(int py=0;py<16;py++)for(int px=0;px<16;px++){
      const uint8_t *src=picture+(py*16+px)*4;if(!src[3])continue;
      uint8_t *dst=out+((y+py)*width+x+px)*4;
      for(int i=0;i<3;i++)dst[i]=src[i]*(disabled?40:100)/100;dst[3]=255;
    }return;
  }
  const uint8_t *ridley=sm_tracker_icon_pixels(0);
  const uint8_t *pal=RomFixedPtr(0x8ee400)+(kind==SM_MODE_VANILLA?240*2:0);
  unsigned brightness=reg_INIDISP&0x80?0:reg_INIDISP&15;
  for(int py=0;py<16;py++)for(int px=0;px<16;px++){
    unsigned rgb[3];
    if(kind==SM_MODE_BOSS_RUSH){const uint8_t *c=ridley+(py*16+px)*4;if(!c[3])continue;for(int i=0;i<3;i++)rgb[i]=c[2-i];}
    else{
      int tile=kind==SM_MODE_VANILLA?0xd0+px/8+(py/8)*16:0x5a;
      const uint32_t address=kind==SM_MODE_VANILLA?0xb6c000:0x8e8000;
      const uint8_t *gfx=RomFixedPtr(address)+tile*32;
      int gx=kind==SM_MODE_VANILLA?px%8:px/2,gy=kind==SM_MODE_VANILLA?py%8:py/2,c=0;
      for(int bit=0;bit<4;bit++)c|=((gfx[gy*2+(bit/2)*16+bit%2]>>(7-gx))&1)<<bit;
      if(!c)continue;uint16_t color=GET_WORD(pal+c*2);
      for(int i=0;i<3;i++)rgb[i]=((color>>((2-i)*5))&31)*255/31;
    }
    uint8_t *dst=out+((y+py)*width+x+px)*4;
    for(int i=0;i<3;i++)dst[i]=rgb[i]*brightness/15*(disabled?40:100)/100;
    dst[3]=255;
  }
}
void sm_generation_render(uint8_t *pixels){
  if(game_state==2 && (game_options_screen_index==2 || game_options_screen_index==3)){
    int disabled=display_mode==SM_MODE_STORY || display_mode==SM_MODE_BOSS_RUSH;
    badge(pixels,256,40,48,display_mode,disabled);badge((uint8_t*)sm_ui_overlay(),256,40,48,display_mode,disabled);
    badge(sm_wide_pixels,400,112,48,display_mode,disabled);badge(sm_wide_hud,400,112,48,display_mode,disabled);
  }else if(sm_slots_mode_badges_active()){
    for(int i=0;i<3;i++){
      int kind=slots[i].mode?SM_MODE_RANDOMIZER:SM_MODE_VANILLA,y=40+i*40;
      badge(pixels,256,88,y,kind,0);badge((uint8_t*)sm_ui_overlay(),256,88,y,kind,0);
      badge(sm_wide_pixels,400,160,y,kind,0);badge(sm_wide_hud,400,160,y,kind,0);
    }
  }
}
