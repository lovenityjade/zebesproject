#include "sm_minimizer.h"
#include "sm_seed.h"
#include "sm_map_exploration.h"
#include "sm_connections.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
#include "sm_minimizer_routing.inc"
_Static_assert(sizeof(SmMinimizerPlan)==152,"Minimizer ABI changed");
static SmMinimizerPlan configurations[4],applied;
static int selected=-1;
static uint8_t *captured,original_data[MINIMIZER_DATA_SIZE],original_doors[40][12],original_cre[40],original_songs[40][6],original_sky[2];
const char *sm_minimizer_catalog_sha256(void){return minimizer_catalog;}
int sm_minimizer_configured(int slot){return slot>=0 && slot<4 && configurations[slot].version;}
int sm_minimizer_active(void){return sm_seed_active() && applied.version;}
int sm_minimizer_destination(int index){return sm_minimizer_active() && index>=0 && index<40?applied.destinations[index]:-1;}
int sm_minimizer_region(int slot,int region){
  if(region<1 || region>10)return 0;
  return slot<0?(sm_minimizer_active() && !!(applied.regions&(1u<<region))):(sm_minimizer_configured(slot) && !!(configurations[slot].regions&(1u<<region)));
}
int sm_minimizer_check(int index){return index>=0 && index<100 && (!sm_minimizer_active() || applied.checks[index]);}
int sm_minimizer_configure(int slot,const SmMinimizerPlan *p,const char *catalog){
  if(slot<0 || slot>=4 || !catalog || strcmp(catalog,minimizer_catalog))return 0;
  if(!p){memset(&configurations[slot],0,sizeof(*p));return 1;}
  if(p->version!=1 || p->size!=sizeof(*p) || !p->regions || p->regions&~0x7fe || p->reserved)return 0;
  uint64_t seen=0;
  for(int i=0;i<40;i++){
    unsigned j=p->destinations[i];
    if(j>=40 || (seen&(UINT64_C(1)<<j)) || p->destinations[j]!=i)return 0;
    seen|=UINT64_C(1)<<j;
  }
  for(int i=0;i<100;i++)if(p->checks[i]>1 || (p->checks[i] && !(p->regions&(1u<<minimizer_locations[i].region)) && !minimizer_locations[i].post_boss))return 0;
  configurations[slot]=*p;return 1;
}
void sm_minimizer_reset(void){memset(configurations,0,sizeof(configurations));memset(&applied,0,sizeof(applied));selected=-1;}
void sm_minimizer_activate(int slot){selected=slot>=0 && slot<4?slot:-1;}
void sm_minimizer_apply(int randomized){applied=randomized && selected>=0?configurations[selected]:(SmMinimizerPlan){0};}
void sm_minimizer_capture(uint8_t *rom){
  captured=rom;
  for(unsigned i=0;i<sizeof(minimizer_spans)/sizeof(*minimizer_spans);i++)
    memcpy(original_data+minimizer_spans[i].offset,rom+minimizer_spans[i].address,minimizer_spans[i].size);
  for(int i=0;i<40;i++){
    memcpy(original_doors[i],rom+0x10000+minimizer_aps[i].door,12);
    original_cre[i]=rom[0x70008+minimizer_aps[i].room];
    for(int n=0;n<minimizer_aps[i].song_count;n++)memcpy(original_songs[i]+n*2,rom+0x70000+minimizer_aps[i].songs[n],2);
  }
  memcpy(original_sky,rom+0x7b7bb,2);
}
void sm_minimizer_restore(uint8_t *rom){
  if(rom!=captured)return;
  for(unsigned i=0;i<sizeof(minimizer_spans)/sizeof(*minimizer_spans);i++)
    memcpy(rom+minimizer_spans[i].address,original_data+minimizer_spans[i].offset,minimizer_spans[i].size);
  for(int i=0;i<40;i++){
    memcpy(rom+0x10000+minimizer_aps[i].door,original_doors[i],12);
    rom[0x70008+minimizer_aps[i].room]=original_cre[i];
    for(int n=0;n<minimizer_aps[i].song_count;n++)memcpy(rom+0x70000+minimizer_aps[i].songs[n],original_songs[i]+n*2,2);
  }
  memcpy(rom+0x7b7bb,original_sky,2);
}
void sm_minimizer_apply_rom(uint8_t *rom){
  if(rom!=captured || !sm_minimizer_active())return;
  for(unsigned i=0;i<sizeof(minimizer_spans)/sizeof(*minimizer_spans);i++)
    memcpy(rom+minimizer_spans[i].address,minimizer_data+minimizer_spans[i].offset,minimizer_spans[i].size);
  for(int i=0;i<40;i++){
    unsigned j=applied.destinations[i];const MixedConnection *c=&minimizer_matrix[i][j];
    uint16_t room=minimizer_aps[j].room,door=minimizer_aps[i].door;
    const uint8_t data[]={room,room>>8,c->flag,c->direction,c->cap_x,c->cap_y,c->screen_x,c->screen_y,c->distance,c->distance>>8,c->asm_ptr,c->asm_ptr>>8};
    memcpy(rom+0x10000+door,data,12);
    if(door==0x93ea || door==0x91ce)rom[0x70008+room]=2;
    if(room==0x93fe){rom[0x7b7bb]=door;rom[0x7b7bc]=door>>8;}
    for(int n=0;n<minimizer_aps[j].song_count;n++){
      uint8_t *p=rom+0x70000+minimizer_aps[j].songs[n];p[0]=minimizer_aps[j].song;p[1]=5;
    }
  }
}
int sm_minimizer_door(void){
  if(!sm_minimizer_active())return 0;
  if(door_def_ptr==0x91da){sm_connections_arrival(0,0,0,0,1);return 1;}
  for(int i=0;i<40;i++)if(minimizer_aps[i].door==door_def_ptr){
    const MixedConnection *c=&minimizer_matrix[i][applied.destinations[i]];
    sm_connections_arrival(c->asm_ptr,c->incompatible,c->x,c->y,c->exit_fix);
    sm_map_exploration_portal(i,applied.destinations[i]);return 1;
  }
  return 0;
}
void sm_minimizer_room(void){
  if(!sm_minimizer_active())return;
  for(unsigned n=0;n<sizeof(minimizer_plms)/sizeof(*minimizer_plms);n++){
    if(room_ptr!=minimizer_plms[n].room || (minimizer_plms[n].state && roomdefroomstate_ptr!=minimizer_plms[n].state))continue;
    const uint8_t *entry=minimizer_plms[n].entry;unsigned header=entry[0]|entry[1]<<8,block=2*(entry[3]*room_width_in_blocks+entry[2]);
    int duplicate=0;for(int i=0;i<40;i++)if(plm_header_ptr[i]==header && plm_block_indices[i]==block){duplicate=1;break;}
    if(duplicate)continue;
    uint8_t previous[6];memcpy(previous,g_ram+0x12,6);memcpy(g_ram+0x12,entry,6);SpawnRoomPLM(0x12);memcpy(g_ram+0x12,previous,6);
  }
}

int sm_minimizer_room_area(int endpoint){return endpoint>=0 && endpoint<40?minimizer_room_areas[endpoint]:-1;}
int sm_minimizer_slot_check(int slot,int index){return slot>=0 && slot<4 && index>=0 && index<100 && configurations[slot].checks[index];}
int sm_minimizer_map_total(int slot,int region){
  if(region<0 || region>=12)return -1;
  return sm_minimizer_region(slot,region)?sm_map_exploration_total(1,region):minimizer_boss_totals[region];
}
int sm_minimizer_enemy_total(int slot,int family){
  if(family<0 || family>=6)return -1;
  int total=0;for(int r=1;r<=10;r++)if(sm_minimizer_region(slot,r))total+=minimizer_enemy_totals[family][r];return total;
}
int sm_minimizer_map_tile(int region,int area,int byte,int mask){
  if(!sm_minimizer_active() || sm_minimizer_region(-1,region))return 1;
  for(unsigned i=0;i<sizeof(minimizer_boss_tiles)/sizeof(*minimizer_boss_tiles);i++)
    if(minimizer_boss_tiles[i].area==area && minimizer_boss_tiles[i].byte==byte && minimizer_boss_tiles[i].mask==mask)return 1;
  return 0;
}
