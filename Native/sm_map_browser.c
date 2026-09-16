#include "sm_map_browser.h"
#include "sm_seed.h"
#include "sm_tracker.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include "sm_rtl.h"
#include "sm_cpu_infra.h"
#include <string.h>
#include <stdio.h>
/* Reuse the original file-load Area Select graphics and palette routines.
 * Never enter its load-station selection or gameplay loading states. The
 * overview borrows menu scratch RAM/VRAM only while gameplay is paused. */
static int mode,selected,dirty,origin_x,origin_y,suppress_start;
static uint8_t pause_ram[0x20000];
static Ppu pause_ppu;
static Dma pause_dma;
static const uint8_t order[]={0,3,5,1,4,2};
static const int label_xy[][2]={{91,50},{42,127},{94,181},{206,80},{206,159},{135,139}};
extern uint8_t sm_wide_hud[];
void sm_map_browser_reset(void){mode=dirty=suppress_start=0;selected=0;}
int sm_map_browser_overview(void){return mode==1 && game_state==15;}
int sm_map_browser_view_area(void){return game_state==15 && mode==2?selected:area_index;}
void sm_map_browser_refresh(void){if(mode==2)dirty=2;}
int sm_map_browser_area_visible(int area){
  if(area<0 || area>=6)return 0;
  if(sm_map_fully_known() || area==area_index || map_station_byte_array[area])return 1;
  const uint8_t *p=(uint8_t*)explored_map_tiles_saved+256*area;
  for(int i=0;i<256;i++)if(p[i])return 1;
  return 0;
}
int sm_map_browser_state(int field){
  if(field==0)return mode;if(field==1)return selected;
  if(field==2){int mask=0;for(int i=0;i<6;i++)if(sm_map_browser_area_visible(i))mask|=1<<i;return mask;}
  if(field==3)return sm_map_browser_view_area();return -1;
}
static void save_pause(void){
  memcpy(pause_ram,g_ram,sizeof(pause_ram));pause_ppu=*g_snes->ppu;pause_dma=*g_snes->dma;
}
static void restore_pause(void){
  /* Preserve elapsed clocks and this frame's real input. All game inventory,
   * room, exploration, coroutine, PPU and DMA state return to the pause state.
   * snes_frame_counter is outside RAM and never rewound. */
  uint8_t input[32],nmi[5],time[8];
  memcpy(input,g_ram+0x87,sizeof(input));memcpy(nmi,g_ram+0x5b5,sizeof(nmi));memcpy(time,g_ram+0x9da,sizeof(time));
  uint16_t held=newly_held_down_timed_held_input,frames=frame_counter_every_frame;
  memcpy(g_ram,pause_ram,sizeof(pause_ram));*g_snes->ppu=pause_ppu;*g_snes->dma=pause_dma;
  memcpy(g_ram+0x87,input,sizeof(input));memcpy(g_ram+0x5b5,nmi,sizeof(nmi));memcpy(g_ram+0x9da,time,sizeof(time));
  newly_held_down_timed_held_input=held;frame_counter_every_frame=frames;
  WriteReg(NMITIMEN,reg_NMITIMEN);WriteReg(HDMAEN,reg_HDMAEN);
  ClearOamExt();
}
static void open_overview(void){
  save_pause();mode=1;
  MapVramForMenu();LoadInitialMenuTiles();LoadMenuPalettes();
  for(int i=0;i<6;i++)if(i==selected)LoadActiveAreaMapForegroundColors(i);else LoadInactiveAreaMapForegroundColors(i);
  reg_BG1HOFS=reg_BG1VOFS=reg_BG3HOFS=reg_BG3VOFS=0;
  reg_BG2HOFS=0;reg_BG2VOFS=24;
  reg_TM=17;reg_TS=4;reg_TMW=reg_TSW=0;
  reg_W12SEL=reg_W34SEL=reg_WOBJSEL=0;
  reg_CGWSEL=2;reg_CGADSUB=0x25;
  reg_HDMAEN=0;WriteReg(HDMAEN,0);
  /* Pause's scanline IRQ draws the HUD over the top 32 lines. Area Select
   * owns the whole native display, including its original title. */
  reg_NMITIMEN&=~0x30;WriteReg(NMITIMEN,reg_NMITIMEN);
  cur_irq_handler=irqhandler_next_handler=0;
  reg_BGMODE=9;WriteReg(BGMODE,9);
  FileSelectMap_2_LoadAreaSelectForegroundTilemap();LoadAreaSelectBackgroundTilemap(selected);
  for(int i=0;i<6;i++)if(order[i]==selected)file_select_map_area_index=i;
  menu_index=6;DrawAreaSelectMapLabels();
}
static void close_detail(void){
  mode=0;dirty=0;
  LoadPauseMenuMapTilemapAndAreaLabel();DetermineMapScrollLimits();
  reg_BG1HOFS=origin_x;reg_BG1VOFS=origin_y;
  map_scrolling_direction=map_scrolling_speed_index=map_scrolling_gear_switch_timer=0;
}
static void draw_detail(void){
  uint16_t real_area=area_index,real_map=has_area_map;
  uint8_t explored[256];memcpy(explored,map_tiles_explored,256);
  if(selected!=real_area)memcpy(map_tiles_explored,(uint8_t*)explored_map_tiles_saved+selected*256,256);
  area_index=selected;has_area_map=map_station_byte_array[selected];
  if(dirty){
    const uint16_t x=reg_BG1HOFS,y=reg_BG1VOFS;
    LoadPauseMenuMapTilemapAndAreaLabel();DetermineMapScrollLimits();
    if(dirty==2){reg_BG1HOFS=x;reg_BG1VOFS=y;}
    else if(selected==real_area){reg_BG1HOFS=origin_x;reg_BG1VOFS=origin_y;}
    else {reg_BG1HOFS=((int)map_min_x_scroll+map_max_x_scroll)/2-128;reg_BG1VOFS=((int)map_min_y_scroll+map_max_y_scroll)/2-120;}
    map_scrolling_direction=map_scrolling_speed_index=map_scrolling_gear_switch_timer=0;dirty=0;
  }
  if(suppress_start){
    if(!(joypad1_lastkeys&kButton_Start))suppress_start=0;
    else newly_held_down_timed_held_input&=~kButton_Start;
  }
  HandlePauseScreenLR();HandlePauseScreenStart();HandleMapScrollArrows();MapScrolling();
  if(selected==real_area)MapScreenDrawSamusPositionIndicator();
  DrawMapIcons();DisplayMapElevatorDestinations();
  area_index=real_area;has_area_map=real_map;memcpy(map_tiles_explored,explored,256);
}
int sm_map_browser_tick(void){
  if(game_state!=15){mode=0;return 0;}
  uint16_t keys=joypad1_newkeys;
  if(mode==1){
    if(!sm_map_browser_area_visible(selected)){
      for(int i=0;i<6;i++)if(order[i]==area_index){SwitchActiveFileSelectMapArea(i);selected=area_index;break;}
    }
    if(keys&kButton_B){restore_pause();close_detail();QueueSfx1_Max6(0x3c);return 0;}
    if(keys&(kButton_A|kButton_Start)){
      restore_pause();mode=2;dirty=1;
      suppress_start=!!(keys&kButton_Start);
      /* Start means select here, never unpause through the same press. */
      newly_held_down_timed_held_input&=~kButton_Start;joypad1_newkeys&=~kButton_Start;
      QueueSfx1_Max6(0x38);draw_detail();return 1;
    }
    int direction=keys&(kButton_Up|kButton_Left)?-1:keys&(kButton_Down|kButton_Right|kButton_Select)?1:0;
    if(direction){
      int i=file_select_map_area_index;
      for(int n=0;n<6;n++){i=(i+direction+6)%6;if(sm_map_browser_area_visible(order[i]))break;}
      if(i!=file_select_map_area_index){SwitchActiveFileSelectMapArea(i);selected=order[i];QueueSfx1_Max6(0x37);}
    }
    DrawAreaSelectMapLabels();return 1;
  }
  if(menu_index || pause_screen_mode || area_index>=6)return 0;
  if(mode==2){
    if(!sm_map_browser_area_visible(selected)){close_detail();return 0;}
    if(keys&(kButton_Start|kButton_R|kButton_L)){close_detail();return 0;}
    if(keys&(kButton_B|kButton_Select)){open_overview();QueueSfx1_Max6(0x3c);return 1;}
    draw_detail();HandleHudTilemap();HandlePauseScreenPaletteAnimation();return 1;
  }
  if(keys&kButton_Select){selected=area_index;origin_x=reg_BG1HOFS;origin_y=reg_BG1VOFS;open_overview();QueueSfx1_Max6(0x38);return 1;}
  return 0;
}
/* Original 8x8 pause alphabet and HUD digits, at their original pixel grid.
 * No generated font, bitmap map reconstruction, or external assets. */
static void pixel(uint8_t *out,int w,int h,int x,int y,uint32_t rgb){
  if(x<0||x>=w||y<0||y>=h)return;
  int b=h==320?15:(g_snes->ppu->forcedBlank?0:g_snes->ppu->brightness);
  uint8_t *p=out+(y*w+x)*4;p[0]=(rgb&255)*b/15;p[1]=((rgb>>8)&255)*b/15;p[2]=(rgb>>16)*b/15;p[3]=255;
}
void sm_native_text_height(uint8_t *out,int w,int h,int x,int y,const char *s,uint32_t color){
  for(;*s;s++,x+=8){
    const uint8_t *gfx;int planes;
    if(*s>='A'&&*s<='Z'){gfx=RomFixedPtr(0xb68000)+(0x30+*s-'A')*32;planes=4;}
    else if(*s>='0'&&*s<='9'){gfx=RomFixedPtr(0x9ab200)+((*s-'0'+9)%10)*16;planes=2;}
    else if(*s==':'){
      for(int dy=2;dy<=5;dy+=3){pixel(out,w,h,x+3,y+dy, color);pixel(out,w,h,x+4,y+dy,color);}continue;
    }
    else continue;
    uint8_t mask[64];
    for(int dy=0;dy<8;dy++)for(int dx=0;dx<8;dx++){
      int c=0;for(int p=0;p<planes;p++)c|=((gfx[dy*2+p/2*16+p%2]>>(7-dx))&1)<<p;
      mask[dy*8+dx]=c==(planes==4?13:2);
    }
    /* A one-pixel black keyline keeps the original glyphs readable over the
     * selected region's bright native tiles, as with the original HUD digits. */
    for(int dy=0;dy<8;dy++)for(int dx=0;dx<8;dx++)if(mask[dy*8+dx])
      for(int oy=-1;oy<=1;oy++)for(int ox=-1;ox<=1;ox++)pixel(out,w,h,x+dx+ox,y+dy+oy,0);
    for(int dy=0;dy<8;dy++)for(int dx=0;dx<8;dx++)if(mask[dy*8+dx])pixel(out,w,h,x+dx,y+dy,color);
  }
}
void sm_native_map_text(uint8_t *out,int w,int x,int y,const char *s,uint32_t color){sm_native_text_height(out,w,224,x,y,s,color);}
static void counts(char *buf,int area){
  int available=sm_tracker_area_count(area,1),total=sm_tracker_area_count(area,3);
  if(available<0)snprintf(buf,24,"WAIT");else snprintf(buf,24,"%02d OF %02d",available,total);
}
static void draw(uint8_t *out,int w){
  int shift=(w-256)/2;char buf[24];
  if(mode==1){
    if(sm_tracker_area_count(0,0)){
      for(int a=0;a<6;a++)if(sm_map_browser_area_visible(a)){
        counts(buf,a);sm_native_map_text(out,w,shift+label_xy[a][0]-(int)strlen(buf)*4,label_xy[a][1]+(a==3?16:8),buf,a==selected?0x20ff20:0xffffff);
      }
      sm_native_map_text(out,w,shift+56,200,"AVAILABLE OF TOTAL",0x20ff20);
    }
    sm_native_map_text(out,w,shift+56,216,"A MAP  B BACK",0xffffff);
  }else if(!menu_index && !pause_screen_mode && area_index<6){
    if(sm_tracker_area_count(0,0)){
      counts(buf,mode==2?selected:area_index);sm_native_map_text(out,w,w-88,216,buf,0x20ff20);
    }
    sm_native_map_text(out,w,8,216,"SELECT AREAS",0xffffff);
  }
}
void sm_map_browser_render(uint8_t *pixels){
  if(game_state!=15)return;
  draw(pixels,256);draw((uint8_t*)sm_ui_overlay(),256);draw(sm_wide_hud,400);
}
