#include "sm_tourian.h"
#include "sm_objectives.h"
#include "sm_objective_events.h"
#include "sm_areas.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
#include "sm_tourian_data.inc"
static uint8_t *captured,original[TOURIAN_DATA_BYTES];
/* Original source scratch word, included in native RAM snapshots. */
#define refill (*(uint16*)(g_ram+0x1ff36))
int sm_tourian_fast(void){return !!(sm_objectives_state(6)&8);}
void sm_tourian_capture(uint8_t *rom){
  captured=rom;unsigned off=0;
  for(unsigned i=0;i<sizeof(tourian_data)/sizeof(tourian_data[0]);i++){
    memcpy(original+off,rom+tourian_data[i].address,tourian_data[i].size);off+=tourian_data[i].size;
  }
}
void sm_tourian_restore(uint8_t *rom){
  if(rom!=captured)return;unsigned off=0;
  for(unsigned i=0;i<sizeof(tourian_data)/sizeof(tourian_data[0]);i++){
    memcpy(rom+tourian_data[i].address,original+off,tourian_data[i].size);off+=tourian_data[i].size;
  }
}
void sm_tourian_apply(uint8_t *rom){
  if(rom!=captured || !sm_tourian_fast())return;
  for(unsigned i=0;i<sizeof(tourian_data)/sizeof(tourian_data[0]);i++)
    memcpy(rom+tourian_data[i].address,tourian_data[i].data,tourian_data[i].size);
}
int sm_tourian_door(void){
  if(!sm_tourian_fast())return 0;
  if(door_def_ptr==0xaa5c){
    sm_objective_event_mark(161);
    if(sm_objective_event(10))opened_door_bit_array[0xa8>>3]|=1<<(0xa8&7);
    return 1;
  }
  if(door_def_ptr==0xaaa4){
    for(unsigned i=2;i<=5;i++)SetEventHappened(i);
    sm_areas_refill();return 1;
  }
  return 0;
}
void sm_tourian_room(void){if(sm_tourian_fast() && room_ptr==0xdd58)refill=0;}
void sm_tourian_frame(void){
  if(!sm_tourian_fast() || game_state!=8 || room_ptr!=0xdd58 || !refill)return;
  unsigned health=samus_health+refill;samus_health=health>samus_max_health?samus_max_health:health;
}
void sm_tourian_hyper_start(void){
  if(!sm_tourian_fast())return;
  flare_counter=flare_animation_frame=flare_slow_sparks_anim_frame=flare_fast_sparks_anim_frame=0;
  flare_animation_timer=flare_slow_sparks_anim_timer=flare_fast_sparks_anim_timer=0;
  samus_special_super_palette_flags=0x8000;
  SomeMotherBrainScripts_3_EnableHyperBeam();
  refill=(uint16)(samus_max_health-samus_health)/314;
  if(!refill)refill=1;
}
void sm_tourian_hyper_end(void){
  if(!sm_tourian_fast())return;
  samus_special_super_palette_flags=special_samus_palette_frame=special_samus_palette_timer=samus_charge_palette_index=0;
  Samus_LoadSuitPalette();refill=0;
  samus_health=samus_max_health;samus_reserve_health=samus_max_reserve_health;
}
