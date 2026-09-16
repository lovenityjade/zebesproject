#include "sm_seed_rules.h"
#include "sm_seed.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include "sm_rtl.h"
#include "sm_movement_pickup_data.inc"
static unsigned selected;
unsigned sm_seed_rules_capabilities(void){return 16383;}
void sm_seed_rules_configure(unsigned mask){selected=mask&16383;}
unsigned sm_seed_rules_active(void){return sm_seed_active()?selected:0;}
int sm_seed_rule(unsigned flag){return (sm_seed_rules_active()&flag)!=0;}
void sm_seed_align_camera(uint16_t *coordinate){
  unsigned low=*coordinate&255;
  if(low>=128)*coordinate+=low==255?1:2;
  else if(low)*coordinate-=low==1?1:2;
}
int sm_seed_crystal_flash(void){
  if(!sm_seed_rule(SM_SEED_ROUND_ROBIN_CF))return 0;
  if((int16)substate<1 || ((int16)samus_missiles<1 && (int16)samus_super_missiles<1 && (int16)samus_power_bombs<1)){
    samus_movement_handler=FUNC16(kSamusMoveHandler_CrystalFlashFinish);
    samus_draw_handler=FUNC16(SamusDrawHandler_Default);samus_anim_frame_timer=3;samus_anim_frame=12;
    return 1;
  }
  if(!(nmi_frame_counter_word&7)){
    uint16 *ammo[]={&samus_missiles,&samus_super_missiles,&samus_power_bombs};
    do{which_item_to_pickup=(which_item_to_pickup+1)%3;}while((int16)*ammo[which_item_to_pickup]<1);
    --*ammo[which_item_to_pickup];Samus_RestoreHealth(50);--substate;
  }
  samus_invincibility_timer=samus_knockback_timer=0;return 1;
}
uint8 sm_original_landing_transition(void);
int sm_seed_landing_eligible(void);
uint8 Samus_HandleTransFromBlockColl_1_0(void){
  uint8 result=sm_original_landing_transition();
  /* Source rando_speed changes only landing poses. Its addition/carry test
   * treats either base-speed word being nonzero as horizontal momentum. */
  if(sm_seed_rule(SM_SEED_RANDO_SPEED) && sm_seed_landing_eligible() && (samus_x_base_speed || samus_x_base_subspeed) && samus_new_pose>=0xa4 && samus_new_pose<=0xa7)
    samus_new_pose=(samus_new_pose&1)?10:9;
  return result;
}

int32_t sm_seed_periodic_damage(int32_t damage){
  if(sm_seed_rule(SM_SEED_BALANCED_SUITS))return (equipped_items&1)?(damage>>2)&0xffff00:damage;
  if(equipped_items&0x20)damage=(damage>>1)&0xffff00;
  if(equipped_items&1)damage=(damage>>1)&0xffff00;
  return damage;
}

const uint16_t *sm_seed_pose_entries(unsigned pose){return respin_entries[respin_offsets[pose<253?pose:0]];}
int sm_seed_pickup_sound(const uint8_t *arguments){
  if(!sm_seed_rule(SM_SEED_ITEM_SOUNDS))return 0;
  uintptr_t offset=(uintptr_t)arguments-(uintptr_t)RomPtr_84(0x8000);
  for(unsigned i=0;i<sizeof(pickup_sounds)/sizeof(*pickup_sounds);i++)if(offset==pickup_sounds[i].address){
    debug_saved_yscroll=2;
    switch(pickup_sounds[i].group){
    case 1:QueueSfx1_Max6(pickup_sounds[i].sound);break;
    case 2:QueueSfx2_Max6(pickup_sounds[i].sound);break;
    case 3:QueueSfx3_Max6(pickup_sounds[i].sound);break;
    }
    return 1;
  }
  return 0;
}
