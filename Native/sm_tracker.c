#include "sm_runs.h"
#include "sm_route.h"
#include "sm_relic.h"
#include "sm_minimizer.h"
#include "sm_tracker.h"
#include "sm_objective_pause.h"
#include "sm_map_browser.h"
#include "sm_seed_atlas.h"
#include "sm_seed.h"
#include "sm_varia_ui.h"
#include "ida_types.h"
#include "sm_rtl.h"
#include "sm_cpu_infra.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
static uint64_t session,revision;
static uint16_t last_slot=0xffff;
static int tracker_enabled,tracker_map=1,tracker_items=1,logic_ready;
static SmTrackerSnapshot logic_input;
static SmObjectiveSnapshot logic_objectives;
static int logic_objectives_valid;
static uint8_t logic_states[100];
static uint8_t boss_states[SM_TRACKER_BOSS_COUNT];
static int bosses_ready;
/* Original room map cells; see Randomizer/sm_tracker_bosses.py. */
static const uint8_t boss_positions[SM_TRACKER_BOSS_COUNT][5]={
  {1,56,20,1,1},{3,19,20,3,1},{4,40,11,4,1},{2,23,18,2,1},
  {5,15,19,5,2},{1,22,3,1,2},{2,15,11,2,2},{4,25,9,4,2},
  {2,19,17,2,4},{0,25,7,0,4}
};
void sm_tracker_new_session(void){sm_route_session();sm_relic_new_session();sm_map_browser_reset();sm_objective_pause_reset();sm_varia_ui_reset();session++;revision=0;last_slot=0xffff;logic_ready=0;}
int sm_tracker_snapshot(SmTrackerSnapshot *out,uint32_t capacity) {
  if(!out || capacity<sizeof(*out) || !g_snes)return 0;
  if(last_slot!=selected_save_slot){session++;last_slot=selected_save_slot;}
  memset(out,0,sizeof(*out));out->version=1;out->size=sizeof(*out);
  out->session=session;out->revision=++revision;out->frame=snes_frame_counter;
  out->slot=selected_save_slot;out->randomized=sm_seed_active();
  out->state=game_state;out->world_valid=game_state==8 || game_state==15;
  out->room=room_ptr;out->area=area_index;
  out->map_x=room_x_coordinate_on_map+(samus_x_pos>>8);
  out->map_y=room_y_coordinate_on_map+(samus_y_pos>>8)+1;
  out->acquired_items=collected_items;out->active_items=equipped_items;
  out->acquired_beams=collected_beams;out->active_beams=equipped_beams;
  out->max_health=samus_max_health;out->max_missiles=samus_max_missiles;
  out->max_supers=samus_max_super_missiles;out->max_power_bombs=samus_max_power_bombs;out->max_reserve=samus_max_reserve_health;
  for(int i=0;i<100;i++)out->collected[i]=sm_seed_location_collected(i);
  memcpy(out->item_bits,item_bit_array,64);memcpy(out->boss_bits,boss_bits_for_area,8);
  memcpy(out->events,events_that_happened,8);memcpy(out->opened_doors,opened_door_bit_array,64);
  memcpy(out->explored_current,map_tiles_explored,256);memcpy(out->explored_saved,explored_map_tiles_saved,2048);
  memcpy(out->map_stations,map_station_byte_array,8);
  if(sm_seed_active())memcpy(out->seed_fingerprint,sm_seed_fingerprint(),65);
  return 1;
}

void sm_tracker_configure(int enabled,int map,int items){
  const int was_known=sm_map_fully_known();
  if(tracker_enabled!=!!enabled)logic_ready=0;
  tracker_enabled=!!enabled;tracker_map=!!map;tracker_items=!!items;
  /* Interface settings can be changed while the native map is already open.
   * Reload its original tilemap immediately, without granting exploration. */
  if(g_snes && was_known!=sm_map_fully_known() && game_state==15 && !pause_screen_mode && !menu_index){
    if(sm_map_browser_state(0)==2){sm_map_browser_refresh();return;}
    const uint16_t x=reg_BG1HOFS,y=reg_BG1VOFS;
    LoadPauseMenuMapTilemapAndAreaLabel();DetermineMapScrollLimits();
    reg_BG1HOFS=x;reg_BG1VOFS=y;
  }
}
static int same_inventory(const SmTrackerSnapshot *a,const SmTrackerSnapshot *b){
  return a->session==b->session && a->slot==b->slot && a->randomized==b->randomized &&
    !memcmp(a->seed_fingerprint,b->seed_fingerprint,65) &&
    a->acquired_items==b->acquired_items && a->acquired_beams==b->acquired_beams &&
    !memcmp(&a->max_health,&b->max_health,10) &&
    !memcmp(a->collected,b->collected,100) && !memcmp(a->boss_bits,b->boss_bits,8) &&
    !memcmp(a->opened_doors,b->opened_doors,64) && !memcmp(a->events,b->events,8);
}
int sm_tracker_publish(const SmTrackerSnapshot *input,const uint8_t *states,uint32_t count){
  SmTrackerSnapshot now;
  if(!input || !states || count!=100 || input->version!=1 || input->size!=sizeof(*input) ||
     !sm_tracker_snapshot(&now,sizeof(now)) || !now.world_valid || !same_inventory(input,&now))return 0;
  for(int i=0;i<100;i++)if(states[i]!=0 && states[i]!=1 && states[i]!=2 && states[i]!=4 && states[i]!=8)return 0;
  memcpy(logic_states,states,100);logic_input=*input;logic_ready=1;logic_objectives_valid=0;bosses_ready=0;return 1;
}
int sm_tracker_publish_objectives(const SmTrackerSnapshot *input,const SmObjectiveSnapshot *objectives,const uint8_t *states,uint32_t count){
  SmObjectiveSnapshot now;
  if(!objectives || objectives->version!=1 || objectives->size!=sizeof(*objectives) ||
     !sm_objectives_snapshot(&now,sizeof(now)) || memcmp(objectives,&now,sizeof(now)) ||
     !sm_tracker_publish(input,states,count))return 0;
  logic_objectives=*objectives;logic_objectives_valid=1;return 1;
}
static int current_logic(const SmTrackerSnapshot *snapshot){
  if(!logic_ready || !same_inventory(&logic_input,snapshot))return 0;
  if(!logic_objectives_valid)return 1;
  SmObjectiveSnapshot now;
  return sm_objectives_snapshot(&now,sizeof(now)) && !memcmp(&logic_objectives,&now,sizeof(now));
}
int sm_tracker_publish_bosses(const SmTrackerSnapshot *input,const uint8_t *states,uint32_t count){
  SmTrackerSnapshot now;
  if(!input || !states || count!=SM_TRACKER_BOSS_COUNT || input->version!=1 || input->size!=sizeof(*input) ||
     !sm_tracker_snapshot(&now,sizeof(now)) || !now.world_valid || !current_logic(&now) || !same_inventory(input,&now))return 0;
  for(unsigned i=0;i<count;i++)if(states[i]!=0 && states[i]!=1 && states[i]!=2 && states[i]!=4 && states[i]!=8 && states[i]!=255)return 0;
  memcpy(boss_states,states,sizeof(boss_states));bosses_ready=1;return 1;
}
/* Compatibility colors requested for the integrated tracker.
 * State bits: reachable=1, blocked=2, sequence break=4, inspect=8. */
uint32_t sm_tracker_color(unsigned state){
  static const uint32_t colors[]={0x3f3f3f,0x20ff20,0xcf1010,0xff9f20,
    0xffff20,0xafff20,0xef5500,0xff9f20,0x3040ff,0x20ffff,0xc010ff,
    0xff9f20,0x20d0d0,0xff9f20,0x20d0d0,0xff9f20};
  return state<16?colors[state]:0xffffff;
}
#include "sm_tracker_assets.inc"
#include "sm_tracker_rom.h"
const uint8_t *sm_tracker_icon_pixels(int id){return id>=0 && id<26?tracker_icons[id]:0;}
int sm_tracker_area_count(int area,int field){
  if(area<0 || area>=6 || field<0 || field>3)return -1;
  if(field==0)return !sm_run_state(0) && tracker_enabled && tracker_map;
  SmTrackerSnapshot s;if(!sm_tracker_snapshot(&s,sizeof(s)) || !s.world_valid)return -1;
  int total=0,left=0,available=0;
  for(int i=0;i<100;i++)if(sm_minimizer_check(i) && tracker_positions[i][0]==area){
    total++;if(!s.collected[i]){left++;available+=logic_states[i]==1;}
  }
  if(field==3)return total;if(field==2)return left;
  if(!tracker_enabled || !tracker_map || !current_logic(&s))return -1;
  return available;
}
extern uint8_t sm_wide_hud[];
static void pixel(uint8_t *out,int width,int x,int y,uint32_t color){
  if(x<0 || x>=width || y<0 || y>=224)return;
  const unsigned brightness=g_snes->ppu->forcedBlank?0:g_snes->ppu->brightness;
  uint8_t *p=out+(y*width+x)*4;
  p[0]=(color&255)*brightness/15;p[1]=((color>>8)&255)*brightness/15;
  p[2]=((color>>16)&255)*brightness/15;p[3]=255;
}
static void square(uint8_t *out,int width,int x,int y,int state,int radius){
  /* Build sectors from semantic state bits. No PopTracker renderer is linked.
   * For two states, keep pairs of adjacent sectors; for three, repeat the
   * reachable/inspect state on the bottom and right. */
  unsigned sectors[4]={0,0,0,0},present[4],count=0;
  const unsigned order[4]={2,4,8,1};
  for(unsigned i=0;i<4;i++)if(state>0 && ((unsigned)state&order[i]))present[count++]=order[i];
  if(count==1)for(unsigned i=0;i<4;i++)sectors[i]=present[0];
  else if(count==2){sectors[0]=sectors[1]=present[0];sectors[2]=sectors[3]=present[1];}
  else if(count>=3){
    sectors[0]=(state&4)?4:8;sectors[1]=(state&2)?2:8;
    sectors[2]=(state&1)?1:8;sectors[3]=count==4?8:sectors[2];
  }
  for(int dy=-radius;dy<=radius;dy++)for(int dx=-radius;dx<=radius;dx++){
    int edge=dx==-radius || dx==radius || dy==-radius || dy==radius;
    int triangle=dy<0 && -dy>=abs(dx)?0:dx<0 && -dx>=abs(dy)?1:dy>=0 && dy>=abs(dx)?2:3;
    uint32_t c=state<0?(edge?0xffffff:0x101018):edge?(state==0?0x3f3f3f:0):sm_tracker_color(sectors[triangle]);
    // Collected markers are smaller hollow gray squares; walls stay visible.
    if(state==0 && !edge)continue;
    pixel(out,width,x+dx,y+dy,c);
  }
}
static void diamond(uint8_t *out,int width,int x,int y,int state,int radius){
  for(int dy=-radius;dy<=radius;dy++)for(int dx=-radius;dx<=radius;dx++){
    const int distance=abs(dx)+abs(dy);if(distance>radius)continue;
    if(state==0 && distance<radius)continue;
    uint32_t color=state<0?(distance==radius?0xffffff:0x101018):distance==radius?(state==0?0x3f3f3f:0):sm_tracker_color(state);
    pixel(out,width,x+dx,y+dy,color);
  }
}
static void icon(uint8_t *out,int width,int x,int y,int id,int bright){
  for(int dy=0;dy<16;dy++)for(int dx=0;dx<16;dx++){
    const uint8_t *p=tracker_icons[id]+(dy*16+dx)*4;if(!p[3])continue;
    unsigned r=p[0],g=p[1],b=p[2];if(!bright){r=r*35/100;g=g*35/100;b=b*35/100;}
    pixel(out,width,x+dx,y+dy,r<<16|g<<8|b);
  }
}
static void number(uint8_t *out,int width,int x,int y,unsigned value){
  /* Tiny capacity counters, same bitmap dimensions as pack overlays at 1x. */
  static const uint16_t digits[]={0x7b6f,0x2492,0x73e7,0x73cf,0x5bc9,0x79cf,0x79ef,0x7249,0x7bef,0x7bcf};
  char text[8];snprintf(text,sizeof(text),"%u",value);int len=strlen(text);
  for(int i=0;i<len;i++)for(int dy=-1;dy<=5;dy++)for(int dx=-1;dx<=3;dx++){
    int on=dx>=0 && dx<3 && dy>=0 && dy<5 && (digits[text[i]-'0']&(1<<(14-dy*3-dx)));
    pixel(out,width,x+i*4+dx,y+dy,on?0xffffff:0);
  }
}
static void inventory_bar(uint8_t *out,int width,const SmTrackerSnapshot *s){
  static const uint16_t bits[]={0,0x1000,1,4,0x100,0,2,0x20,0x1000,0x200,0,1,2,0x2000,0,4,0x4000,8,0,8,0x8000};
  static const uint8_t beam[]={0,1,0,0,0,0,1,0,0,0,0,1,0,0,0,1,0,0,0,1,0};
  const int origin=width==400?16:0,spacing=width==400?20:16;
  for(int y=0;y<32;y++)for(int x=0;x<width;x++)pixel(out,width,x,y,0);
  for(int i=0;i<21;i++){
    int bright=bits[i]?!!((beam[i]?s->acquired_beams:s->acquired_items)&bits[i]):1;
    if(i==0)bright=!(s->boss_bits[2]&1);if(i==5)bright=!(s->boss_bits[3]&1);
    if(i==10)bright=!(s->boss_bits[1]&1);if(i==14)bright=!(s->boss_bits[4]&1);if(i==18)bright=!(s->boss_bits[5]&2);
    icon(out,width,origin+(i%11)*spacing,(i/11)*16,i,bright);
  }
  const unsigned counts[]={s->max_missiles,s->max_supers,s->max_power_bombs,
    s->max_health>=99?(s->max_health-99)/100:0,s->max_reserve/100};
  for(int i=0;i<5;i++){
    int x=width==400?256+(i<3?i*40:(i-3)*60):178+(i<3?i*26:(i-3)*39),y=i<3?0:16;
    icon(out,width,x,y,21+i,counts[i]!=0);number(out,width,x+13,y+10,counts[i]);
  }
  if(sm_relic_required()){sm_relic_icon(out,width,origin+10*spacing,16);number(out,width,origin+10*spacing+10,26,sm_relic_count());}
}
static void draw_tracker(uint8_t *out,int width,const SmTrackerSnapshot *s){
  const int pause=game_state==15;
  if(pause && pause_screen_mode!=0)return;
  if(pause && tracker_enabled && tracker_items)inventory_bar(out,width,s);
  const int logical=tracker_enabled && tracker_map;
  if(!logical && !sm_varia_ui_active(SM_VARIA_MARKERS))return;
  const int atlas=pause && sm_seed_atlas_active();
  int fresh=current_logic(s);
  for(int draw_area=0;draw_area<6;draw_area++){
  if(!atlas && draw_area!=(pause?sm_map_browser_view_area():area_index))continue;
  int groups[32][64];memset(groups,0xff,sizeof(groups));
  for(int i=0;i<100;i++){
    if(!sm_minimizer_check(i))continue;
    const uint8_t *p=tracker_positions[i];if(p[0]!=draw_area)continue;
    if(!pause && !sm_seed_atlas_local(p[0],p[1],p[2]))continue;
    int state=s->collected[i]?0:!logical?16:fresh?logic_states[i]:-2;
    if(groups[p[2]][p[1]]==-1)groups[p[2]][p[1]]=state;
    else if(state==-2 || groups[p[2]][p[1]]==-2)groups[p[2]][p[1]]=-2;
    else groups[p[2]][p[1]]|=state;
  }
  for(int y=0;y<32;y++)for(int x=0;x<64;x++){
    int state=groups[y][x];if(state==-1)continue;
    int dx,dy,r=state==0 || state==16?1:3;
    if(pause){
      if(atlas){
        if(!sm_seed_atlas_project(draw_area,x*8+4,y*8+4,width,&dx,&dy))continue;
        if(r>sm_seed_atlas_cell_size()/2-1)r=sm_seed_atlas_cell_size()/2-1;
        if(r<1)r=1;
      }else{dx=x*8+4-(int16_t)reg_BG1HOFS+(width-256)/2;dy=y*8+4-(int16_t)reg_BG1VOFS;}
      if(dx-r<8 || dx+r>=width-8 || dy-r<48 || dy+r>=192)continue;
    }else{
      int mx=x-s->map_x+(width==400?4:2),my=y-s->map_y+1;
      if(mx<0 || mx>=(width==400?7:5) || my<0 || my>=(width==400?4:3))continue;
      dx=(width==400?336:208)+mx*8+4;dy=my*8+4;
      // Keep the player cursor readable even when standing on a check.
      if(x==s->map_x && y==s->map_y)r=2;
    }
    if(state==16){for(int y=-1;y<=1;y++)for(int x=-1;x<=1;x++)pixel(out,width,dx+x,dy+y,0xffffff);}
    else square(out,width,dx,dy,state,r);
    if(!pause && x==s->map_x && y==s->map_y)pixel(out,width,dx,dy,0xffffff);
  }
  if(logical && fresh && bosses_ready)for(int i=0;i<SM_TRACKER_BOSS_COUNT;i++){
    const uint8_t *p=boss_positions[i];if(p[0]!=draw_area || boss_states[i]==255)continue;
    if(!pause && !sm_seed_atlas_local(p[0],p[1],p[2]))continue;
    const int x=p[1],y=p[2],state=(s->boss_bits[p[3]]&p[4])?0:boss_states[i];
    const int shared=groups[y][x]!=-1;
    int dx,dy,r=state==0?1:shared?2:3;
    if(pause){
      if(atlas){
        if(!sm_seed_atlas_project(draw_area,x*8+4,y*8+4,width,&dx,&dy))continue;
        if(r>sm_seed_atlas_cell_size()/2-1)r=sm_seed_atlas_cell_size()/2-1;
        if(r<1)r=1;
      }else{dx=x*8+4-(int16_t)reg_BG1HOFS+(width-256)/2;dy=y*8+4-(int16_t)reg_BG1VOFS;}
    }else{
      const int mx=x-s->map_x+(width==400?4:2),my=y-s->map_y+1;
      if(mx<0 || mx>=(width==400?7:5) || my<0 || my>=(width==400?4:3))continue;
      dx=(width==400?336:208)+mx*8+4;dy=my*8+4;
      if(x==s->map_x && y==s->map_y)r=2;
    }
    /* Bomb Torizo shares a cell with its item: retain both independent colors. */
    if(shared){dx-=2;dy-=2;}
    if(pause && (dx-r<8 || dx+r>=width-8 || dy-r<48 || dy+r>=192))continue;
    diamond(out,width,dx,dy,state,r);
    if(!pause && x==s->map_x && y==s->map_y){
      pixel(out,width,(width==400?336:208)+(width==400?4:2)*8+4,12,0xffffff);
    }
  }
  } // physical areas share one presentation projection
}
void sm_tracker_render(uint8_t *native_pixels){
  if(sm_run_state(0))return;
  SmTrackerSnapshot s;
  if((!tracker_enabled && !sm_varia_ui_active(SM_VARIA_MARKERS)) || !sm_tracker_snapshot(&s,sizeof(s)) || !s.world_valid)return;
  draw_tracker(native_pixels,256,&s);draw_tracker((uint8_t*)sm_ui_overlay(),256,&s);draw_tracker(sm_wide_hud,400,&s);
}

int sm_tracker_reveals_map(void){return !sm_run_state(0) && tracker_enabled && tracker_map;}
