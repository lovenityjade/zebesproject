#include "sm_minimizer.h"
#include "sm_areas.h"
#include "sm_connections.h"
#include "sm_map_exploration.h"
#include "sm_seed.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
#include "sm_areas.inc"
#include "sm_areas_rom_assets.inc"
#include "sm_room_rom_loader.h"
void sm_areas_load_assets(void){sm_room_rom_load(area_data,sizeof(area_data));}
typedef struct {uint8_t targets[AREA_AP_COUNT],enabled;} AreaRouting;
static AreaRouting configurations[4],applied;
static int selected=-1;
static uint8_t *captured;
static uint8_t original_data[AREA_DATA_SIZE];
static uint8_t original_doors[AREA_AP_COUNT][12],original_cre[AREA_AP_COUNT];
static uint8_t original_songs[AREA_AP_COUNT][6],original_sky[2];

const char *sm_areas_catalog_sha256(void){return area_catalog;}
int sm_areas_configured(int slot){return slot>=0 && slot<4 && (configurations[slot].enabled || sm_minimizer_configured(slot));}
int sm_areas_destination(int index){return sm_seed_active() && applied.enabled && !sm_minimizer_active() && index>=0 && index<AREA_AP_COUNT?applied.targets[index]:-1;}
int sm_areas_configure(int slot,const uint8_t *destinations,int count,const char *catalog){
  if(slot<0 || slot>=4 || (count!=0 && count!=AREA_AP_COUNT) || !catalog || strcmp(catalog,area_catalog) || (count && !destinations))return 0;
  AreaRouting next={0};uint32_t seen=0;
  for(int i=0;i<count;i++){
    unsigned j=destinations[i];
    if(j>=AREA_AP_COUNT || (seen&(UINT32_C(1)<<j)) || destinations[j]!=i)return 0;
    seen|=UINT32_C(1)<<j;next.targets[i]=j;
  }
  next.enabled=count!=0;configurations[slot]=next;return 1;
}
void sm_areas_reset(void){memset(configurations,0,sizeof(configurations));memset(&applied,0,sizeof(applied));selected=-1;}
void sm_areas_activate(int slot){selected=slot>=0 && slot<4?slot:-1;}
void sm_areas_capture(uint8_t *rom){
  captured=rom;memset(&applied,0,sizeof(applied));
  for(unsigned i=0;i<sizeof(area_spans)/sizeof(area_spans[0]);i++)
    memcpy(original_data+area_spans[i].offset,rom+area_spans[i].address,area_spans[i].size);
  for(int i=0;i<AREA_AP_COUNT;i++){
    memcpy(original_doors[i],rom+0x10000+area_aps[i].door,12);
    original_cre[i]=rom[0x70008+area_aps[i].room];
    for(int n=0;n<area_aps[i].song_count;n++)memcpy(original_songs[i]+n*2,rom+0x70000+area_aps[i].songs[n],2);
  }
  memcpy(original_sky,rom+0x7b7bb,2);
}
void sm_areas_restore(uint8_t *rom){
  if(rom!=captured)return;
  memset(&applied,0,sizeof(applied));
  for(unsigned i=0;i<sizeof(area_spans)/sizeof(area_spans[0]);i++)
    memcpy(rom+area_spans[i].address,original_data+area_spans[i].offset,area_spans[i].size);
  for(int i=0;i<AREA_AP_COUNT;i++){
    memcpy(rom+0x10000+area_aps[i].door,original_doors[i],12);
    rom[0x70008+area_aps[i].room]=original_cre[i];
    for(int n=0;n<area_aps[i].song_count;n++)memcpy(rom+0x70000+area_aps[i].songs[n],original_songs[i]+n*2,2);
  }
  memcpy(rom+0x7b7bb,original_sky,2);
}
void sm_areas_apply_base(uint8_t *rom,int randomized){
  if(rom!=captured)return;
  applied=randomized && selected>=0?configurations[selected]:(AreaRouting){0};
  if(randomized && sm_minimizer_active())applied.enabled=1;
  if(!applied.enabled)return;
  for(unsigned i=0;i<sizeof(area_spans)/sizeof(area_spans[0]);i++)
    if(area_spans[i].phase==0)memcpy(rom+area_spans[i].address,area_data+area_spans[i].offset,area_spans[i].size);
}
void sm_areas_apply_routing(uint8_t *rom){
  if(rom!=captured || !applied.enabled)return;
  /* Source getStartDoors applies these replacements after layout/tweaks.
   * In particular Le Coude's enemy-count byte overrides moat.ips. */
  for(unsigned i=0;i<sizeof(area_spans)/sizeof(area_spans[0]);i++)
    if(area_spans[i].phase==1)memcpy(rom+area_spans[i].address,area_data+area_spans[i].offset,area_spans[i].size);
  if(sm_minimizer_active())return;
  for(int i=0;i<AREA_AP_COUNT;i++){
    unsigned j=applied.targets[i];const AreaConnection *c=&area_matrix[i][j];
    uint16_t room=area_aps[j].room,door=area_aps[i].door;
    const uint8_t bytes[]={room,room>>8,c->flag,c->direction,c->cap_x,c->cap_y,c->screen_x,c->screen_y,c->distance,c->distance>>8,c->asm_ptr,c->asm_ptr>>8};
    memcpy(rom+0x10000+door,bytes,12);
    if(door==0x93ea)rom[0x70008+room]=2;
    if(room==0x93fe){rom[0x7b7bb]=door;rom[0x7b7bc]=door>>8;}
    for(int n=0;n<area_aps[j].song_count;n++){
      uint8_t *p=rom+0x70000+area_aps[j].songs[n];p[0]=area_aps[j].song;p[1]=5;
    }
  }
}
int sm_areas_door(void){
  if(!sm_seed_active() || !applied.enabled)return 0;
  for(int i=0;i<AREA_AP_COUNT;i++)if(area_aps[i].door==door_def_ptr){
    const AreaConnection *c=&area_matrix[i][applied.targets[i]];
    sm_connections_arrival(c->asm_ptr,c->incompatible,c->x,c->y,c->exit_fix);
    sm_map_exploration_portal(i,applied.targets[i]);
    return 1;
  }
  return 0;
}
void sm_areas_room(void){
  if(!sm_seed_active() || !applied.enabled)return;
  for(unsigned n=0;n<sizeof(area_plms)/sizeof(area_plms[0]);n++){
    if(room_ptr!=area_plms[n].room || (area_plms[n].state && roomdefroomstate_ptr!=area_plms[n].state))continue;
    const uint8_t *entry=area_plms[n].entry;
    unsigned header=entry[0]|entry[1]<<8,block=2*(entry[3]*room_width_in_blocks+entry[2]);
    int duplicate=0;
    for(int i=0;i<40;i++)if(plm_header_ptr[i]==header && plm_block_indices[i]==block){duplicate=1;break;}
    if(duplicate)continue;
    uint8_t previous[6];memcpy(previous,g_ram+0x12,6);
    memcpy(g_ram+0x12,entry,6);SpawnRoomPLM(0x12);memcpy(g_ram+0x12,previous,6);
  }
}
void sm_areas_refill(void){
  if(!sm_seed_active() || !applied.enabled)return;
  samus_health=samus_max_health;samus_reserve_health=samus_max_reserve_health;
  samus_missiles=samus_max_missiles;samus_super_missiles=samus_max_super_missiles;
  samus_power_bombs=samus_max_power_bombs;
}
int sm_areas_test_destination(int index,int *room,int *door,int *x,int *y){
  if(!applied.enabled || index<0 || index>=AREA_AP_COUNT)return 0;
  int j=applied.targets[index];const AreaConnection *c=&area_matrix[index][j];
  *room=area_aps[j].room;*door=area_aps[index].door;*x=c->x;*y=c->y;return 1;
}
