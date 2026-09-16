#include "sm_animals.h"
#include "sm_mirror.h"
#include "sm_minimizer.h"
#include "sm_escape.h"
#include "sm_tourian.h"
#include "sm_scavenger.h"
#include "sm_objectives.h"
#include "sm_world_data.h"
#include "sm_seed.h"
#include "sm_indicators.h"
#include "sm_connections.h"
#include "sm_doors.h"
#include "sm_areas.h"
#include "sm_map_exploration.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
#include "sm_world_data.inc"
#include "sm_world_rom_assets.inc"

#include "sm_room_rom_loader.h"
void sm_world_data_load_assets(void){sm_room_rom_load(world_patch_bytes,sizeof(world_patch_bytes));}

static uint8_t original[WORLD_BACKUP_SIZE];
static uint64_t selected,applied;
static uint8_t order[64];
static int order_count;
static uint8_t *captured_rom;
int sm_world_chozo_without_spacejump(void){
  return sm_seed_active() && (applied&WORLD_LN_CHOZO)!=0;
}
int sm_world_torizo_wakes(void){
  /* VARIA's btcheck reads $1C83: the original item PLM header. Room PLMs
   * allocate backwards, so this is index 38, not array element 1. Removing
   * that PLM after ANY pickup wakes the statue and closes its grey door.
   * The source-written sleep flag leaves it dormant for Nothing when the
   * Chozo-robots objective is absent. Old plans retain their prior flags. */
  if(sm_seed_active() && (applied&WORLD_BOMB_TORIZO))
    return !(sm_objectives_state(6)&SM_OBJECTIVE_BT_SLEEP) && plm_header_ptr[38]==0;
  return (collected_items&0x1000)!=0;
}
int sm_world_data_catalog_supported(const char *sha256){
  if(!sha256)return 0;
  for(unsigned i=0;i<sizeof(world_compatible_catalogs)/sizeof(world_compatible_catalogs[0]);i++)
    if(!strcmp(sha256,world_compatible_catalogs[i]))return 1;
  return 0;
}
const char *sm_world_data_catalog_sha256(void){return world_catalog_sha256;}
uint64_t sm_world_data_capabilities(void){return WORLD_VALID_MASK;}
int sm_world_data_configure(uint64_t selection){
  if(selection&~WORLD_VALID_MASK)return 0;
  order_count=0;
  for(int i=0;i<64;i++)if(selection&(UINT64_C(1)<<i))order[order_count++]=i;
  selected=selection;return 1;
}
int sm_world_data_configure_order(const uint8_t *ids,int count){
  if(count<0 || count>64 || (count && !ids))return 0;
  uint64_t next=0;
  for(int i=0;i<count;i++){
    if(ids[i]>=64)return 0;
    uint64_t bit=UINT64_C(1)<<ids[i];
    if(!(WORLD_VALID_MASK&bit) || (next&bit))return 0;
    next|=bit;
  }
  if(count)memcpy(order,ids,count);order_count=count;selected=next;return 1;
}
int sm_world_data_get_order(uint8_t *ids,int capacity){
  if(capacity<order_count || (order_count && !ids))return -1;
  if(order_count)memcpy(ids,order,order_count);return order_count;
}
uint64_t sm_world_data_selected(void){return selected;}
void sm_world_data_capture(uint8_t *rom){
  captured_rom=rom;
  sm_animals_capture(rom);
  sm_mirror_capture(rom);
  sm_indicators_capture(rom);
  sm_connections_capture(rom);
  sm_doors_capture(rom);
  sm_areas_capture(rom);
  sm_map_exploration_capture(rom);
  sm_scavenger_capture(rom);
  sm_tourian_capture(rom);
  sm_minimizer_capture(rom);
  sm_escape_capture(rom);
  applied=0;
  for(unsigned i=0;i<sizeof(world_spans)/sizeof(world_spans[0]);i++)
    memcpy(original+world_spans[i].offset,rom+world_spans[i].address,world_spans[i].size);
}
void sm_world_data_apply(uint8_t *rom,int randomized){
  if(rom!=captured_rom)return;
  applied=randomized?selected:0;
  sm_animals_restore(rom);
  sm_mirror_restore(rom);
  sm_escape_restore(rom);
  sm_minimizer_restore(rom);
  sm_tourian_restore(rom);
  sm_scavenger_restore(rom);
  sm_doors_restore(rom);
  sm_areas_restore(rom);
  sm_indicators_restore(rom);
  sm_connections_restore(rom);
  /* Restore every overlapping span from the pristine capture first. Applying
   * another mask cannot retain tiles, PLMs or save entries from a previous seed. */
  for(unsigned i=0;i<sizeof(world_spans)/sizeof(world_spans[0]);i++)
    memcpy(rom+world_spans[i].address,original+world_spans[i].offset,world_spans[i].size);
  sm_mirror_apply(randomized);
  sm_mirror_apply_rom(rom);
  sm_animals_apply(rom,randomized);
  sm_indicators_apply(rom,!!(applied&WORLD_DOOR_INDICATORS));
  sm_connections_apply(rom,randomized);
  sm_map_exploration_apply(rom,randomized);
  sm_objectives_apply(randomized);
  sm_scavenger_apply(randomized);
  sm_minimizer_apply(randomized);
  sm_escape_apply(randomized);
  if(!randomized){sm_escape_apply_rom(rom);sm_seed_refresh_maps();return;}
  sm_areas_apply_base(rom,randomized);
  for(int n=0;n<order_count;n++){
    unsigned p=order[n];
    for(unsigned i=world_patches[p].first;i<world_patches[p].first+world_patches[p].count;i++)
      memcpy(rom+world_spans[i].address,world_patch_bytes+world_spans[i].offset,world_spans[i].size);
  }
  sm_areas_apply_routing(rom);
  sm_doors_apply(rom,randomized);
  sm_scavenger_apply_rom(rom);
  sm_minimizer_apply_rom(rom);
  sm_tourian_apply(rom);
  sm_escape_apply_rom(rom);
  sm_seed_refresh_maps();
}
