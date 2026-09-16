#include "sm_travel.h"
#include "sm_generation.h"
#include "sm_map_browser.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include "sm_rtl.h"
#include <string.h>
#include <stdio.h>
static uint8_t enabled[3];
static const uint8_t order[]={0,3,5,1,4,2};
static const int labels[][2]={{91,50},{42,127},{94,181},{206,80},{206,159},{135,139}};
extern uint8_t sm_wide_hud[];
void sm_travel_reset(void){memset(enabled,0,sizeof(enabled));}
void sm_travel_configure(int slot,int value){if(slot>=0 && slot<3)enabled[slot]=!!value;}
int sm_travel_enabled(void){
  int slot=sm_slots_current();
  return slot>=0 && slot<3 && enabled[slot] && sm_seed_active() && sm_generation_state()==2;
}
static const uint8_t *icons(int area){return RomPtr_82(GET_WORD(RomPtr_82(0xc80b+2*area)));}
int sm_travel_station_mask(int area){
  if(!sm_travel_enabled() || area<0 || area>=6)return 0;
  /* Lower byte: saves; upper byte: elevators. Never expose debug stations,
   * elevator spawns, or destinations just because the full map is revealed. */
  unsigned unlocked=used_save_stations_and_elevators[2*area],result=0;
  if(area==sram_area_index && sram_save_station_index<8)unlocked|=1u<<sram_save_station_index;
  const uint8_t *xy=icons(area);
  unsigned list=GET_WORD(RomFixedPtr(0x80c4b5)+area*2);
  for(int i=0;i<8;i++){
    unsigned x=GET_WORD(xy+i*4),y=GET_WORD(xy+i*4+2);
    if(x==0xffff)break;
    if(!(unlocked&(1u<<i)) || x>=0xfffe || y>=0xfffe)continue;
    unsigned room=GET_WORD(RomPtr_80(list+14*i));
    if(room>=0x8000 && RomPtr_8F(room)[1]==area)result|=1u<<i;
  }
  return result;
}
static int choose(int area,int start,int direction){
  unsigned mask=sm_travel_station_mask(area);
  for(int n=0;n<8;n++){start=(start+direction+8)%8;if(mask&(1u<<start))return start;}
  return -1;
}
int sm_travel_area_input(void){
  if(!sm_travel_enabled() || game_state!=5)return 0;
  unsigned keys=joypad1_newkeys;
  int index=file_select_map_area_index<6?file_select_map_area_index:0;
  if(!sm_travel_station_mask(order[index])){
    for(int i=0;i<6;i++)if(sm_travel_station_mask(order[i])){SwitchActiveFileSelectMapArea(i);index=i;break;}
  }
  if(keys&kButton_B){menu_index=22;DrawAreaSelectMapLabels();return 1;}
  int direction=keys&(kButton_Up|kButton_Left)?-1:keys&(kButton_Select|kButton_Down|kButton_Right)?1:0;
  if(direction){
    int next=index;
    for(int n=0;n<6;n++){next=(next+direction+6)%6;if(sm_travel_station_mask(order[next]))break;}
    if(sm_travel_station_mask(order[next]) && next!=index){SwitchActiveFileSelectMapArea(next);QueueSfx1_Max6(0x37);}
  }else if(keys&(kButton_A|kButton_Start)){
    int area=order[index],station=choose(area,7,1);
    if(area==sram_area_index && sram_save_station_index<8 && (sm_travel_station_mask(area)&(1u<<sram_save_station_index)))station=sram_save_station_index;
    if(station>=0){area_index=area;load_station_index=station;++menu_index;QueueSfx1_Max6(0x38);}
    else QueueSfx1_Max6(0x3d);
  }
  DrawAreaSelectMapLabels();return 1;
}
int sm_travel_room_input(void){
  if(!sm_travel_enabled() || game_state!=5)return 0;
  unsigned keys=joypad1_newkeys;
  /* Let the original B transition and A/Start load path run unchanged.
   * Validate the destination again before the original confirmation. */
  if(keys&kButton_B)return 0;
  if(keys&(kButton_A|kButton_Start)){
    if(load_station_index<8 && (sm_travel_station_mask(area_index)&(1u<<load_station_index)))return 0;
    QueueSfx1_Max6(0x3d);DrawFileSelectMapIcons();return 1;
  }
  int direction=keys&kButton_L?-1:keys&(kButton_R|kButton_Select)?1:0;
  if(!direction)return 0;
  int next=choose(area_index,load_station_index,direction);
  if(next>=0 && next!=load_station_index){
    load_station_index=next;
    /* Original initializer positions the original blinking Samus marker and
     * scrolls to this station. It changes RAM only; loading never autosaves. */
    menu_index=9;FileSelectMap_9_InitRoomSelectMap();QueueSfx1_Max6(0x37);
  }
  DrawFileSelectMapIcons();return 1;
}
void sm_travel_draw_stations(void){
  if(!sm_travel_enabled() || game_state!=5 || area_index>=6)return;
  unsigned mask=sm_travel_station_mask(area_index);const uint8_t *xy=icons(area_index);
  for(int i=0;i<8;i++)if(mask&(1u<<i))
    DrawMenuSpritemap(8,GET_WORD(xy+4*i)-reg_BG1HOFS,GET_WORD(xy+4*i+2)-reg_BG1VOFS,1024);
}
static void draw(uint8_t *out,int w){
  int shift=(w-256)/2;char text[32];
  if(menu_index==6){
    for(int area=0;area<6;area++){
      unsigned mask=sm_travel_station_mask(area);if(!mask)continue;
      int count=0;for(int i=0;i<8;i++)count+=!!(mask&(1u<<i));
      snprintf(text,sizeof(text),"%d SAVE%s",count,count==1?"":"S");
      sm_native_map_text(out,w,shift+labels[area][0]-(int)strlen(text)*4,labels[area][1]+(area==3?16:8),text,0x20ff20);
    }
    sm_native_map_text(out,w,shift+40,200,"SAVED STATIONS ONLY",0x20ff20);
    sm_native_map_text(out,w,shift+40,216,"A REGION  B BACK",0xffffff);
  }else if(menu_index==10){
    unsigned mask=sm_travel_station_mask(area_index);int count=0,rank=0;
    for(int i=0;i<8;i++)if(mask&(1u<<i)){++count;if(i==load_station_index)rank=count;}
    snprintf(text,sizeof(text),"STATION %d OF %d",rank,count);
    sm_native_map_text(out,w,shift+64,192,text,0x20ff20);
    sm_native_map_text(out,w,shift+16,208,"L R STATION  A START",0xffffff);
    sm_native_map_text(out,w,shift+104,216,"B BACK",0xffffff);
  }
}
void sm_travel_render(uint8_t *pixels){
  if(!sm_travel_enabled() || game_state!=5)return;
  draw(pixels,256);draw((uint8_t*)sm_ui_overlay(),256);draw(sm_wide_hud,400);
}
