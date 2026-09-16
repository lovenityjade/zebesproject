#include "sm_escape.h"
#include "sm_relic.h"
#include "sm_seed.h"
#include "sm_objective_events.h"
#include "sm_map_exploration.h"
#include "sm_connections.h"
#include "sm_minimizer.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
_Static_assert(sizeof(SmEscapeClockPlan)==56,"Escape clock ABI changed");
static SmEscapeClockPlan configurations[4],applied;
#include "sm_escape_data.inc"
#include "sm_escape_rom_assets.inc"
#include "sm_room_rom_loader.h"
void sm_escape_load_assets(void){sm_room_rom_load(escape_room_data,sizeof(escape_room_data));}
#include "sm_escape_routing.inc"
_Static_assert(sizeof(SmEscapeRoutingPlan)==44,"Escape routing ABI changed");
static SmEscapeRoutingPlan routes[4],routing;
static uint8_t original_doors[ESCAPE_ENDPOINTS][12],original_room_data[ESCAPE_ROOM_DATA_SIZE];
static uint8_t *captured,original_populations[6][19];
static int selected=-1;
#define escape_cycle (*(uint16*)(g_ram+0x1ff34))
#define half_value (*(uint16*)(g_ram+0x1ff4e))
void RoomCode_ExplodeShakes(void);
void DoorCode_SetScroll_48(void);
static int valid_bcd(uint16_t n){return (n&15)<10 && ((n>>4)&15)<6 && ((n>>8)&15)<10 && (n>>12)<10;}
const char *sm_escape_catalog_sha256(void){return escape_catalog;}
int sm_escape_routing_configure(int slot,const SmEscapeRoutingPlan *p,const char *catalog){
  if(slot<0 || slot>=4 || !catalog || strcmp(catalog,escape_catalog))return 0;
  if(!p){memset(&routes[slot],0,sizeof(*p));return 1;}
  if(p->version!=1 || p->size!=sizeof(*p) || p->count<6 || p->count>16 || p->animals>4)return 0;
  for(int i=0;i<16;i++){
    if(i<p->count){if(p->pairs[i][0]>=ESCAPE_ENDPOINTS || p->pairs[i][1]>=ESCAPE_ENDPOINTS)return 0;}
    else if(p->pairs[i][0] || p->pairs[i][1])return 0;
  }
  /* Every generated animal list must be initialized; never expose placeholders. */
  for(int door=0xadac;door<=0xadf4;door+=24){
    int found=0;for(int i=0;i<p->count;i++)found|=escape_aps[p->pairs[i][0]].door==door;
    if(!found)return 0;
  }
  int found=0;for(int i=0;i<p->count;i++)found|=escape_aps[p->pairs[i][0]].door==0xae00;
  if(!found)return 0;
  routes[slot]=*p;return 1;
}
int sm_escape_clock_configure(int slot,const SmEscapeClockPlan *p){
  if(slot<0 || slot>=4)return 0;
  if(!p){memset(&configurations[slot],0,sizeof(*p));return 1;}
  if(p->version!=1 || p->size!=sizeof(*p) || p->flags&~7 || p->reserved ||
     (p->flags&3)==3 || !valid_bcd(p->timer) || !valid_bcd(p->half_timer))return 0;
  int regional=!!(p->flags&SM_ESCAPE_DISABLED_TOURIAN);
  if(regional!=(p->timer==0) || (regional && p->half_timer) || (!regional && p->half_timer>p->timer))return 0;
  for(int i=0;i<10;i++)if(!valid_bcd(p->timers[i]) || !valid_bcd(p->half_timers[i]) ||
    p->half_timers[i]>p->timers[i] || (regional && !p->timers[i]) || (!regional && (p->timers[i] || p->half_timers[i])))return 0;
  configurations[slot]=*p;return 1;
}
void sm_escape_reset(void){memset(configurations,0,sizeof(configurations));memset(&applied,0,sizeof(applied));memset(routes,0,sizeof(routes));memset(&routing,0,sizeof(routing));selected=-1;}
void sm_escape_activate(int slot){selected=slot>=0 && slot<4?slot:-1;}
void sm_escape_apply(int randomized){
  applied=randomized && selected>=0?configurations[selected]:(SmEscapeClockPlan){0};
  routing=applied.version && selected>=0?routes[selected]:(SmEscapeRoutingPlan){0};
}
int sm_escape_enabled(void){return sm_seed_active() && applied.version;}
int sm_escape_configured(int slot){return slot>=0 && slot<4 && configurations[slot].version && routes[slot].version;}
int sm_escape_slot_flags(int slot){return sm_escape_configured(slot)?configurations[slot].flags:-1;}
int sm_escape_map_offset(int slot,int region){
  if(slot<0?(!sm_escape_enabled() || !routing.version):!sm_escape_configured(slot))return 0;
  if((slot<0?sm_minimizer_active():sm_minimizer_configured(slot)) && region>=0 && !sm_minimizer_region(slot,region))return 0;
  int count=0;
  for(unsigned i=0;i<sizeof(escape_map_tiles)/sizeof(*escape_map_tiles);i++){
    int r=escape_map_tiles[i].region;
    if(region>=0 && region!=r)continue;
    if((slot<0?sm_minimizer_active():sm_minimizer_configured(slot)) && !sm_minimizer_region(slot,r))continue;
    count++;
  }
  return count;
}
int sm_escape_map_tile(int area,int byte,int mask){
  if(!sm_escape_enabled() || !routing.version)return 1;
  for(unsigned i=0;i<sizeof(escape_map_tiles)/sizeof(*escape_map_tiles);i++)
    if(escape_map_tiles[i].area==area && escape_map_tiles[i].byte==byte && escape_map_tiles[i].mask==mask)return 0;
  return 1;
}
int sm_escape_active(void){return sm_escape_enabled() && CheckEventHappened(14);}
int sm_escape_extended(void){
  if(!sm_escape_active() || area_index==5)return 0;
  if(area_index!=0)return 1;
  return room_ptr!=0x91f8 && room_ptr!=0x92fd && room_ptr!=0x9879 && room_ptr!=0x9804 && room_ptr!=0x96ba;
}
int sm_escape_nothing_allowed(void){
  return !sm_escape_enabled() || (applied.flags&(SM_ESCAPE_DISABLED_TOURIAN|SM_ESCAPE_NOTHING_CRATERIA))!=5 || sm_map_exploration_region()==1;
}
static void timer_gfx(void){
  VramWriteEntry *p=gVramWriteEntry(vram_write_queue_tail);
  p->size=0x400;p->src.addr=0xc000;p->src.bank=0xb0;p->vram_dst=0x7e00;vram_write_queue_tail+=7;
}
void sm_escape_setup(void){
  if(!sm_escape_enabled())return;
  escape_cycle=0;
  memset(opened_door_bit_array,255,32);memset(boss_bits_for_area,255,8);
  SetEventHappened(11);sm_objective_event_mark(160);
}
uint16_t sm_escape_timer(void){
  if(!sm_escape_enabled())return 0x300;
  if(applied.timer){half_value=applied.half_timer;return applied.timer;}
  int region=sm_map_exploration_region();
  if(region<1 || region>10){half_value=0x130;return 0x300;}
  half_value=applied.half_timers[region-1];return applied.timers[region-1];
}
int sm_escape_trigger(void){
  if(!sm_escape_enabled() || !(applied.flags&SM_ESCAPE_DISABLED_TOURIAN) || sm_escape_active())return 0;
  sm_escape_setup();earthquake_type=0x15;earthquake_timer=0xffff;
  CallSomeSamusCode(15);timer_gfx();timer_status=2;
  memset(music_queue_track,0,16);memset(music_queue_delay,0,16);
  music_queue_read_pos=music_queue_write_pos;music_timer=music_entry=0;
  QueueMusic_Delayed8(0);QueueMusic_Delayed8(0xff24);QueueMusic_Delayed8(7);
  SetEventHappened(14);return 1;
}
int sm_escape_hyper(unsigned projectile){
  return sm_escape_active() && !(applied.flags&SM_ESCAPE_DISABLED_TOURIAN) && projectile<10 && (projectile_type[projectile]&8);
}
void sm_escape_room_setup(void){
  if(sm_escape_enabled() && routing.version){
    if(roomdefroomstate_ptr==0x98c4)door_list_pointer=0xf000+(escape_cycle&3)*4;
    if(roomdefroomstate_ptr==0x984f)door_list_pointer=0xf046;
    if(roomdefroomstate_ptr==0xcb22 && sm_escape_active())DoorCode_SetScroll_48();
    if(room_ptr==0xcc6f){
      const uint8_t entry[]={0x48,0xc8,1,6,0x61,routing.animals==4?0x10:0x90};
      uint8_t previous[6];memcpy(previous,g_ram+0x12,6);memcpy(g_ram+0x12,entry,6);SpawnRoomPLM(0x12);memcpy(g_ram+0x12,previous,6);
    }
  }
  if(!sm_escape_extended())return;
  earthquake_type=*(uint16*)&timer_seconds<half_value?0x18:0x15;earthquake_timer=0xffff;timer_gfx();
}
void sm_escape_room_main(void){if(sm_escape_extended())RoomCode_ExplodeShakes();}
void sm_escape_load_header(void){
  if(sm_relic_escape_active()){room_music_data_index=room_music_track_index=0;timer_gfx();return;}
  if(!sm_escape_extended())return;
  room_music_data_index=room_music_track_index=0;
  if(!(applied.flags&SM_ESCAPE_REMOVE_ENEMIES))return;
  room_enemy_population_ptr=0x85a9;room_enemy_tilesets_ptr=0x80eb;
  for(unsigned i=0;i<sizeof(escape_enemies)/sizeof(*escape_enemies);i++)if(room_ptr==escape_enemies[i].room){
    room_enemy_population_ptr=escape_enemies[i].population;room_enemy_tilesets_ptr=escape_enemies[i].graphics;break;
  }
}

int sm_escape_door(void){
  if(!sm_escape_enabled())return 0;
  if(door_def_ptr==0xaaec){sm_escape_setup();return 1;}
  /* Match the final writer entry when a physical source is repeated. */
  for(int i=(int)routing.count-1;i>=0;i--){
    unsigned src=routing.pairs[i][0],dst=routing.pairs[i][1];
    if(escape_aps[src].door!=door_def_ptr)continue;
    const EscapeConnection *c=&escape_matrix[src][dst];
    sm_connections_arrival(c->asm_ptr,c->incompatible,c->x,c->y,0);
    if(c->cycle && sm_escape_active())escape_cycle=(escape_cycle+1)&3;
    return 1;
  }
  return 0;
}

void sm_escape_capture(uint8_t *rom){
  captured=rom;for(int i=0;i<6;i++)memcpy(original_populations[i],rom+escape_populations[i].address,19);
  for(int i=0;i<ESCAPE_ENDPOINTS;i++)memcpy(original_doors[i],rom+0x10000+escape_aps[i].door,12);
  for(unsigned i=0;i<sizeof(escape_room_spans)/sizeof(*escape_room_spans);i++)
    memcpy(original_room_data+escape_room_spans[i].offset,rom+escape_room_spans[i].address,escape_room_spans[i].size);
}
void sm_escape_restore(uint8_t *rom){
  if(rom!=captured)return;
  for(int i=0;i<6;i++)memcpy(rom+escape_populations[i].address,original_populations[i],19);
  for(int i=0;i<ESCAPE_ENDPOINTS;i++)memcpy(rom+0x10000+escape_aps[i].door,original_doors[i],12);
  for(unsigned i=0;i<sizeof(escape_room_spans)/sizeof(*escape_room_spans);i++)
    memcpy(rom+escape_room_spans[i].address,original_room_data+escape_room_spans[i].offset,escape_room_spans[i].size);
}
void sm_escape_apply_rom(uint8_t *rom){
  if(rom!=captured)return;
  if(sm_escape_enabled())for(int i=0;i<6;i++)memcpy(rom+escape_populations[i].address,escape_populations[i].data,19);
  if(!sm_escape_enabled() || !routing.version)return;
  for(unsigned i=0;i<sizeof(escape_room_spans)/sizeof(*escape_room_spans);i++)
    memcpy(rom+escape_room_spans[i].address,escape_room_data+escape_room_spans[i].offset,escape_room_spans[i].size);
  const uint32_t animal_doors[]={0,0x784bd,0x78b0b,0x7c54c};
  if(routing.animals>0 && routing.animals<4)rom[animal_doors[routing.animals]]=0x10;
  for(int i=0;i<routing.count;i++){
    unsigned src=routing.pairs[i][0],dst=routing.pairs[i][1];
    const EscapeConnection *c=&escape_matrix[src][dst];uint16_t room=escape_aps[dst].room;
    const uint8_t data[]={room,room>>8,c->flag,c->direction,c->cap_x,c->cap_y,c->screen_x,c->screen_y,c->distance,c->distance>>8,c->asm_ptr,c->asm_ptr>>8};
    memcpy(rom+0x10000+escape_aps[src].door,data,12);
  }
}
