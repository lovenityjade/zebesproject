#include "sm_animals.h"
#include "sm_mirror.h"
#include "sm_minimizer.h"
#include "sm_escape.h"
#include "sm_tourian.h"
#include "sm_scavenger.h"
#include "sm_objectives.h"
#include "sm_start.h"
#include "sm_world_data.h"
#include "sm_seed.h"
#include "sm_connections.h"
#include "sm_doors.h"
#include "sm_areas.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include "sm_rtl.h"
#include <string.h>
#include "sm_start.inc"
typedef struct {uint16_t spawn;uint8_t patches[64],count;SmDoorIndicator indicators[SM_INDICATOR_MAX];int indicator_count;} Start;
static Start configurations[4];
typedef struct {uint16_t spawn;uint8_t doors[16],count;} InitialDoors;
static InitialDoors initial_configurations[4];
static int active=-1;
static int definition(unsigned spawn){
  for(unsigned i=0;i<sizeof(starts)/sizeof(starts[0]);i++)if(starts[i].spawn==spawn)return i;
  return -1;
}
void sm_start_reset(void){memset(configurations,0,sizeof(configurations));memset(initial_configurations,0,sizeof(initial_configurations));active=-1;sm_world_data_configure(0);sm_indicators_select(0,0);sm_connections_reset();sm_doors_reset();sm_areas_reset();sm_objectives_reset();sm_scavenger_reset();sm_minimizer_reset();sm_escape_reset();sm_mirror_reset();sm_animals_reset();}
int sm_start_configure_initial_doors(int slot,int spawn,const uint8_t *doors,int count){
  if(slot<0 || slot>=4 || count<0 || count>16 || (count && !doors))return 0;
  InitialDoors next={0};
  if(count){
    int index=definition(spawn);if(index<0)return 0;
    uint8_t expected[256]={0},seen[256]={0};int total=0;
    for(unsigned i=0;i<sizeof(initial_base_doors);i++)expected[initial_base_doors[i]]=1;
    for(int i=0;i<starts[index].door_count;i++)expected[starts[index].doors[i]]=1;
    for(int i=0;i<256;i++)total+=expected[i];
    if(count!=total)return 0;
    for(int i=0;i<count;i++){if(!expected[doors[i]] || seen[doors[i]])return 0;seen[doors[i]]=1;}
    next.spawn=spawn;next.count=count;memcpy(next.doors,doors,count);
  }
  initial_configurations[slot]=next;return 1;
}
int sm_start_configure(int slot,int spawn,const uint8_t *patches,int count,const char *catalog){
  return sm_start_configure_world(slot,spawn,patches,count,catalog,0,0);
}
int sm_start_configure_world(int slot,int spawn,const uint8_t *patches,int count,const char *catalog,const SmDoorIndicator *indicators,int indicator_count){
  int index=definition(spawn);
  if(slot<0 || slot>=4 || index<0 || count<0 || count>64 || (count && !patches) || !sm_world_data_catalog_supported(catalog))return 0;
  if(starts[index].area_only && !sm_areas_configured(slot))return 0;
  uint64_t mask=0;
  for(int i=0;i<count;i++){
    if(patches[i]>=64)return 0;
    uint64_t bit=UINT64_C(1)<<patches[i];
    if(!(bit&sm_world_data_capabilities()) || (mask&bit))return 0;
    mask|=bit;
  }
  if(starts[index].save_patch>=0 && !(mask&(UINT64_C(1)<<starts[index].save_patch)))return 0;
  if(!sm_indicators_validate(indicators,indicator_count,mask))return 0;
  for(int i=0;i<indicator_count;i++)if(sm_doors_plm_direction(indicators[i].plm,1)>=0 && !sm_doors_configured(slot))return 0;
  Start next={0};next.spawn=spawn;next.count=count;if(count)memcpy(next.patches,patches,count);
  next.indicator_count=indicator_count;if(indicator_count)memcpy(next.indicators,indicators,indicator_count*sizeof(*indicators));
  configurations[slot]=next;return 1;
}
int sm_start_spawn(int slot){return slot>=0 && slot<4?configurations[slot].spawn:-1;}
int sm_start_active_slot(void){return active;}
int sm_start_activate(int slot,int enabled){
  if(!enabled){active=-1;sm_indicators_select(0,0);sm_connections_activate(-1);sm_doors_activate(-1);sm_areas_activate(-1);sm_objectives_activate(-1);sm_scavenger_activate(-1);sm_minimizer_activate(-1);sm_escape_activate(-1);sm_mirror_activate(-1);sm_animals_activate(-1);return sm_world_data_configure_order(0,0);}
  if(slot<0 || slot>=4)return 0;
  Start *s=&configurations[slot];
  int index=definition(s->spawn);
  if(index<0 || (starts[index].area_only && !sm_areas_configured(slot)))return 0;
  if(initial_configurations[slot].count && initial_configurations[slot].spawn!=s->spawn)return 0;
  for(int i=0;i<s->indicator_count;i++)if(sm_doors_plm_direction(s->indicators[i].plm,1)>=0 && !sm_doors_configured(slot))return 0;
  if(!sm_objectives_validate_slot(slot))return 0;
  if(!sm_world_data_configure_order(s->patches,s->count))return 0;
  sm_indicators_select(s->indicators,s->indicator_count);
  sm_connections_activate(slot);sm_doors_activate(slot);
  sm_areas_activate(slot);sm_objectives_activate(slot);sm_scavenger_activate(slot);sm_minimizer_activate(slot);sm_escape_activate(slot);sm_mirror_activate(slot);sm_animals_activate(slot);
  active=slot;return 1;
}
void sm_start_initialize_save(void){
  sm_scavenger_new_game();
  const InitialDoors *initial=active>=0?&initial_configurations[active]:0;
  if(initial && initial->count){
    for(int i=0;i<initial->count;i++){unsigned bit=initial->doors[i];opened_door_bit_array[bit>>3]|=1<<(bit&7);}
  }else sm_doors_initialize_save();
  unsigned spawn=active>=0?configurations[active].spawn:0;
  area_index=spawn==0xfffe?6:spawn>>8;load_station_index=spawn==0xfffe?0:spawn&255;
  if(spawn && spawn!=0xfffe){
    SetEventHappened(0);
    // A new alternate start must not inherit the historical ship-start door.
    opened_door_bit_array[0x32>>3]&=~(1<<(0x32&7));
  }
  int index=definition(spawn);
  if(index>=0)for(int i=0;i<starts[index].door_count;i++){
    unsigned bit=starts[index].doors[i];opened_door_bit_array[bit>>3]|=1<<(bit&7);
  }
  if(area_index<6 && load_station_index<16){
    unsigned offset=area_index*2;uint16_t mask=GET_WORD(used_save_stations_and_elevators+offset)|(1u<<load_station_index);
    used_save_stations_and_elevators[offset]=mask;used_save_stations_and_elevators[offset+1]=mask>>8;
  }
}
void sm_start_new_game(void){
  sm_start_initialize_save();
  loading_game_state=area_index==6?31:5;game_state=6;game_options_screen_index=menu_option_index=0;
}
void sm_start_room(void){
  sm_tourian_room();
  sm_indicators_room();
  sm_connections_room();
  sm_areas_room();
  sm_minimizer_room();
  if(!sm_seed_active() || active<0)return;
  int index=definition(configurations[active].spawn);if(index<0 || !starts[index].save_room || starts[index].save_room!=room_ptr)return;
  unsigned block=2*(starts[index].save_y*room_width_in_blocks+starts[index].save_x);
  for(int i=0;i<40;i++)if(plm_header_ptr[i]==0xb76f && plm_block_indices[i]==block)return;
  /* SpawnRoomPLM has a native RAM descriptor path. Preserve its scratch bytes;
   * the existing setup receives the correct save index before running. */
  uint8_t previous[6];memcpy(previous,g_ram+0x12,6);
  const uint8_t entry[]={0x6f,0xb7,starts[index].save_x,starts[index].save_y,starts[index].save_index,0};
  memcpy(g_ram+0x12,entry,6);SpawnRoomPLM(0x12);memcpy(g_ram+0x12,previous,6);
}
void sm_start_door(void){
  /* wh_open_tube.ips: door $A498 calls $8FF400, sets event $0B, returns.
   * Execute its native equivalent; never install its 65816 routine bytes. */
  if(sm_seed_active() && active>=0 && configurations[active].spawn==0x0407 && door_def_ptr==0xa498)SetEventHappened(11);
}
