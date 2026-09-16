#include "sm_objective_pause.h"
#include "sm_objectives.h"
#include "sm_objective_events.h"
#include "sm_map_browser.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include "sm_rtl.h"
#include "sm_cpu_infra.h"
#include <stdio.h>
#include <string.h>
#include "sm_ui_rom_assets.h"
#include "sm_objective_pause_assets.inc"
void sm_objective_pause_load_assets(void){objective_pause_gfx_load();}
enum {OBJ_PAGE=11,MAP_OUT=12,OBJ_LOAD=13,OBJ_IN=14,OBJ_OUT=15,MAP_LOAD=16};
static uint16_t tiles[2048],wide_tiles[2048],saved_palette[128],saved_footer[64];
static uint8_t saved_gfx[0x4000];
static int resources,first,last;
static int enabled(void){return sm_objectives_state(0)>0;}
void sm_objective_pause_reset(void){resources=first=last=0;}
int sm_objective_pause_state(int field){return field==0?resources:field==1?first:field==2?last:field==3?menu_index==OBJ_PAGE && pause_screen_mode==2:0;}
const uint16_t *sm_objective_pause_wide(void){return wide_tiles;}
static int offset(int x,int y){return (x&31)+y*32+(x>=32?1024:0);}
static void word(uint16_t *map,int x,int y,uint16_t tile){map[offset(x,y+5)]=tile;}
static int text(uint16_t *map,int x,int y,const char *s){
  for(;*s;s++,x++)word(map,x,y,*s==' '?0x2800:(unsigned char)*s<128?objective_pause_chars[(unsigned char)*s]:0);
  return x;
}
static void compose(uint16_t *map,int width,const SmObjectiveSnapshot *s){
  memset(map,0,4096);
  for(int y=0;y<19;y++)for(int x=0;x<width;x++){
    uint16_t tile=0;
    if(width==32)tile=objective_pause_base[y*32+x];
    else if(y==0){int sx=x-(width-32)/2;if(sx>=0 && sx<32)tile=objective_pause_base[sx];}
    else if(y==1 || y==3){int sx=x<=13?x:x==width-2?30:x==width-1?31:14;tile=objective_pause_base[y*32+sx];}
    else if(y==2){int sx=x<=21?x:x>=width-10?x-width+32:-1;if(sx>=0)tile=objective_pause_base[y*32+sx];}
    else if(y==4 || y==18){int sx=x<2?x:x==width-2?30:x==width-1?31:2;tile=objective_pause_base[y*32+sx];}
    else {int sx=x<30?x:x==width-2?30:x==width-1?31:-1;if(sx>=0)tile=objective_pause_base[y*32+sx];}
    word(map,x,y,tile);
  }
  char buffer[48];snprintf(buffer,sizeof(buffer),"%-2d",s->state[3]);text(map,2,2,buffer);
  text(map,width-10,2,(s->state[6]&SM_OBJECTIVE_DISABLED_TOURIAN)?"Disabled":(s->state[6]&SM_OBJECTIVE_FAST_TOURIAN)?"    Fast":" Vanilla");
  if(!s->state[7]){text(map,7,12," Golden Statues Room.");return;}
  for(int y=5;y<18;y++)for(int x=2;x<width-2;x++)word(map,x,y,0);
  const int center=width/2;
  if(first){word(map,center-1,5,0x39b8);word(map,center,5,0x79b8);}
  int row=6;last=first-1;
  for(int i=first;i<s->state[0] && row<18;i++,row+=2){
    int id=s->goals[i],complete=s->values[i][0],have=s->values[i][1],total=s->values[i][2],pct=s->values[i][3];
    if(id<0 || id>=59)break;
    const int start=objective_pause_labels[id].start_event;
    const int progress=objective_pause_labels[id].progress && !complete
      && (!start || sm_objective_event(start))
      && (!objective_pause_labels[id].nonzero || have);
    word(map,2,row,complete?0x2576:progress && (have || start)?0x2577:0);
    char number[8];snprintf(number,sizeof(number),"%d.",i+1);
    snprintf(buffer,sizeof(buffer),"%-3s%s",number,objective_pause_labels[id].text);
    int x=text(map,3,row,buffer);
    if(progress && total){
      if(pct>=0)snprintf(buffer,sizeof(buffer)," (%d%%)",pct);
      else snprintf(buffer,sizeof(buffer)," (%d/%d)",have,total);
      /* Keep every original glyph on-grid when a three-digit counter would
       * cross the frame. Native 4:3 wraps the counter onto the spare row. */
      if(x+(int)strlen(buffer)>width-2)text(map,6,row+1,buffer+1);
      else text(map,x,row,buffer);
    }
    last=i;
  }
  if(last+1<s->state[0]){word(map,center-1,17,0xb9b8);word(map,center,17,0xf9b8);}
}
static void update(void){
  SmObjectiveSnapshot snapshot;sm_objectives_snapshot(&snapshot,sizeof(snapshot));
  compose(tiles,32,&snapshot);compose(wide_tiles,50,&snapshot);
  memcpy(g_snes->ppu->vram+0x3000,tiles,sizeof(tiles));
}
static void restore(void){
  if(!resources)return;
  memcpy(g_snes->ppu->vram,saved_gfx,sizeof(saved_gfx));
  memcpy(ram3000.pause_menu_map_tilemap+800,saved_footer,sizeof(saved_footer));
  memcpy(g_snes->ppu->vram+0x3800+800,saved_footer,sizeof(saved_footer));
  memcpy(palette_buffer,saved_palette,sizeof(saved_palette));resources=0;
}
void sm_objective_pause_unpause(void){restore();}
static void label(int x,const uint16_t *top,int palette){
  for(int i=0;i<5;i++){
    ram3000.pause_menu_map_tilemap[805+x+i]=(top[i]&~0x1c00)|palette;
    uint16_t bottom=top[i]+0x10;
    if((top[i]&1023)>=0x1f8 && (top[i]&1023)<=0x1fa)bottom=top[i]+3;
    ram3000.pause_menu_map_tilemap[837+x+i]=(bottom&~0x1c00)|palette;
  }
}
int sm_objective_pause_buttons(void){
  if(!enabled() || pausemenu_button_label_mode==1)return 0;
  if(resources)memcpy(ram3000.pause_menu_map_tilemap+800,RomFixedPtr(0xb6e640),sizeof(saved_footer));
  static const uint16_t obj[]={0x2899,0x29f8,0x29f9,0x29fa,0x289d};
  static const uint16_t map[]={0x2899,0x289a,0x289b,0x289c,0x289d};
  static const uint16_t samus[]={0x2879,0x287a,0x287b,0x287c,0x287d};
  /* Relocate VARIA's OBJ glyphs: its original indices replace the native
   * START label. These six slots are unused by the map and by VARIA text;
   * restore them for the original equipment tilemap which uses two of them. */
  for(int i=0;i<3;i++)for(int row=0;row<2;row++){
    int source=0x96+i+row*16,tile=0x1f8+i+row*3;
    const uint8_t *gfx=pausemenu_button_label_mode==2?RomFixedPtr(0xb68000)+tile*32:objective_pause_gfx+source*32;
    memcpy(g_snes->ppu->vram+tile*16,gfx,32);
  }
  if(pausemenu_button_label_mode==2){SetPauseScreenButtonLabelPalettes_2();label(0,map,0x800);label(17,samus,0x1400);}
  else {SetPauseScreenButtonLabelPalettes_0();label(0,obj,pausemenu_button_label_mode==3?0x1400:0x800);label(17,pausemenu_button_label_mode==3?map:samus,0x800);}
  if(resources)for(int i=800;i<864;i++){
    uint16_t tile=ram3000.pause_menu_map_tilemap[i];
    for(unsigned j=0;j<sizeof(objective_pause_footer_tiles)/sizeof(objective_pause_footer_tiles[0]);j++){
      if((tile&1023)==objective_pause_footer_tiles[j][0]){
        ram3000.pause_menu_map_tilemap[i]=(tile&~1023)|objective_pause_footer_tiles[j][1];break;
      }
    }
  }
  return 1;
}
int sm_objective_pause_highlights(void){
  if(!enabled() || (pausemenu_button_label_mode!=0 && pausemenu_button_label_mode!=3))return 0;
  /* Native shoulder animation tables have only map/start/equipment entries.
   * Map now has two available shoulders; objectives has only its right one.
   * Never index those tables with our additional objective label mode. */
  uint16_t mode=pausemenu_button_label_mode;
  pausemenu_button_label_mode=mode==0?2:0;DrawPauseScreenSpriteAnim(2,0x18,0xd0);
  pausemenu_button_label_mode=0;DrawPauseScreenSpriteAnim(2,0xe8,0xd0);
  pausemenu_button_label_mode=mode;return 1;
}
int sm_objective_pause_input(void){
  if(!enabled())return 0;
  if(newly_held_down_timed_held_input&kButton_Start)return 1;
  const int left=newly_held_down_timed_held_input&kButton_L,right=newly_held_down_timed_held_input&kButton_R;
  if((left && pausemenu_button_label_mode==0) || (right && pausemenu_button_label_mode==3)){
    pausemenu_start_lr_pressed_highlight_timer=5;pausemenu_shoulder_button_highlight=left?1:2;
    menu_index=left?MAP_OUT:OBJ_OUT;pausemenu_button_label_mode=left?3:0;
    SetPauseScreenButtonLabelPalettes();QueueSfx1_Max6(0x38);return 1;
  }
  return pausemenu_button_label_mode==3;
}
int sm_objective_pause_tick(void){
  if(menu_index<OBJ_PAGE || menu_index>MAP_LOAD || !enabled())return 0;
  switch(menu_index){
    case OBJ_PAGE:
      HandlePauseScreenLR();HandlePauseScreenStart();
      if(menu_index!=OBJ_PAGE || game_state!=15)break;
      if(sm_objectives_state(7) && (joypad1_newkeys&(kButton_Up|kButton_Down))){
        int direction=joypad1_newkeys&kButton_Up?-1:1;
        if((direction<0 && first>0) || (direction>0 && last+1<sm_objectives_state(0))){first+=direction;QueueSfx1_Max6(0x37);}
        else QueueSfx1_Max6(0x36);
      }
      update();pause_screen_mode=2;break;
    case MAP_OUT:
      DisplayMapElevatorDestinations();MapScreenDrawSamusPositionIndicator();DrawMapIcons();
      HandlePauseMenuLRPressHighlight();HandleFadeOut();
      if(reg_INIDISP==0x80){EnableNMI();screen_fade_delay=screen_fade_counter=0;menu_index=OBJ_LOAD;}break;
    case OBJ_LOAD:
      reg_BG4HOFS=reg_BG1HOFS;reg_BG4VOFS=reg_BG1VOFS;reg_BG1HOFS=reg_BG1VOFS=0;
      memcpy(saved_gfx,g_snes->ppu->vram,sizeof(saved_gfx));memcpy(saved_palette,palette_buffer,sizeof(saved_palette));resources=1;
      memcpy(saved_footer,ram3000.pause_menu_map_tilemap+800,sizeof(saved_footer));
      memcpy(g_snes->ppu->vram,objective_pause_gfx,sizeof(objective_pause_gfx));memcpy(palette_buffer,objective_pause_palettes,sizeof(saved_palette));
      for(unsigned i=0;i<sizeof(objective_pause_footer_tiles)/sizeof(objective_pause_footer_tiles[0]);i++){
        int source=objective_pause_footer_tiles[i][0],tile=objective_pause_footer_tiles[i][1];
        memcpy(g_snes->ppu->vram+tile*16,RomFixedPtr(0xb68000)+source*32,32);
      }
      first=0;pause_screen_mode=2;update();SetPauseScreenButtonLabelPalettes();
      screen_fade_delay=screen_fade_counter=1;menu_index=OBJ_IN;break;
    case OBJ_IN:
      pause_screen_mode=2;HandleFadeIn();
      if(reg_INIDISP==15){screen_fade_delay=screen_fade_counter=0;menu_index=OBJ_PAGE;pausemenu_button_label_mode=3;}break;
    case OBJ_OUT:
      HandlePauseMenuLRPressHighlight();HandleFadeOut();
      if(reg_INIDISP==0x80){EnableNMI();screen_fade_delay=screen_fade_counter=0;menu_index=MAP_LOAD;}break;
    case MAP_LOAD:
      restore();pause_screen_mode=0;menu_index=6;PauseMenu_6_EquipmentToMap_Load();break;
  }
  return 1;
}
