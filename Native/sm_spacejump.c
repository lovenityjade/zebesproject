#include "sm_runs.h"
#include "sm_bridge.h"
#include "sm_spacejump.h"
#include "sm_seed_rules.h"
#include "ida_types.h"
#include "variables.h"
/* Assist state lives outside native RAM. Original mode leaves the upstream
 * movement routine and its input/velocity checks unchanged. */
static int enabled=1,buffer,age;
static uint16 previous_frame,previous_room;
void sm_spacejump_reset(void){buffer=age=0;previous_frame=previous_room=0;}
void sm_set_assisted_spacejump(int value){enabled=!!value;sm_spacejump_reset();}
int sm_assisted_spacejump(void){return enabled && sm_run_assists_allowed() && !sm_seed_rule(SM_SEED_INFINITE_SPACEJUMP);}
void sm_spacejump_frame(void) {
  if(game_state!=8 || samus_movement_type!=3 || time_is_frozen_flag || previous_room!=room_ptr)
    sm_spacejump_reset();
}
void sm_spacejump_tick(void) {
  if(!sm_assisted_spacejump())return;
  if((uint16)(snes_frame_counter-previous_frame)!=1 || previous_room!=room_ptr)buffer=age=0;
  previous_frame=snes_frame_counter;previous_room=room_ptr;
  ++age;if(buffer)--buffer;
  /* Ignore the take-off press. A distinct press can wait up to 16 frames
   * for the apex. Holding jump cannot turn into automatic repeated jumps. */
  if(age>2 && (joypad1_newkeys&button_config_jump_a))buffer=16;
}
int sm_spacejump_request(void) {
  if(!enabled || !sm_run_assists_allowed() || !(equipped_items&0x200) || game_state!=8 || time_is_frozen_flag)return 0;
  if(buffer && samus_y_dir==2) {buffer=0;return 1;}
  return 0;
}
