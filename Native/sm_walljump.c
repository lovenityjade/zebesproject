#include "sm_runs.h"
#include "sm_walljump.h"
#include "ida_types.h"
#include "sm_rtl.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
#include <stdlib.h>

uint8 sm_original_wall_jump_check(int32 amt);
static int enabled=1,buffer,contact,cooldown,push,away,age;
static uint16_t previous_frame,wall_x,wall_y;
static uint8_t probe_ram[0x20000];
void sm_walljump_reset(void) {buffer=contact=cooldown=push=away=age=0;previous_frame=0;}
void sm_walljump_mode(int assisted) {enabled=!!assisted;sm_walljump_reset();}
int sm_walljump_assisted(void) {return enabled && sm_run_assists_allowed();}
uint16_t sm_walljump_input(uint16_t buttons) {
  if(!enabled || !sm_run_assists_allowed())return buttons;
  if(game_state!=8 || (samus_movement_type!=3 && samus_movement_type!=20)) {
    sm_walljump_reset();return buttons;
  }
  if(push>0) {--push;buttons=(buttons&~(64|128))|(away<0?64:128);}
  return buttons;
}
static int wall_on_side(int side) {
  // Collision routines use RAM scratch registers. Probe without leaving those
  // side effects in the simulation, then use the original wall-jump transition.
  memcpy(probe_ram,g_ram,sizeof probe_ram);
  samus_collision_direction=side<0?0:1;
  CheckEnemyColl_Result e=Samus_CheckSolidEnemyColl(6*65536);
  int hit=e.collision;
  if(!hit) {WallJumpBlockCollDetect(side*6*65536);hit=samus_collision_flag;}
  memcpy(g_ram,probe_ram,sizeof probe_ram);
  return hit;
}
uint8 Samus_WallJumpCheck(int32 amt) {
  if(!enabled || !sm_run_assists_allowed())return sm_original_wall_jump_check(amt);
  if((uint16_t)(snes_frame_counter-previous_frame)!=1)buffer=contact=cooldown=age=0;
  previous_frame=snes_frame_counter;
  ++age;
  if(buffer)--buffer;if(contact)--contact;if(cooldown)--cooldown;
  // The initial take-off press is not a request to bounce off a nearby wall.
  if(age>5 && (joypad1_newkeys&button_config_jump_a))buffer=9;
  if(samus_movement_type!=3 && samus_movement_type!=20)return sm_original_wall_jump_check(amt);
  int left=wall_on_side(-1),right=wall_on_side(1);
  if(left!=right) {away=left?1:-1;contact=7;wall_x=samus_x_pos;wall_y=samus_y_pos;}
  if(buffer && contact && !cooldown && abs((int)samus_x_pos-wall_x)<=8 && abs((int)samus_y_pos-wall_y)<=16) {
    samus_pose_x_dir=away<0?4:8;
    joypad1_lastkeys=(joypad1_lastkeys&~(kButton_Left|kButton_Right))|(away<0?kButton_Left:kButton_Right);
    input_to_pose_calc=5;enemy_index_to_shake=0xffff;
    buffer=contact=0;cooldown=12;push=6;
    return 1;
  }
  return sm_original_wall_jump_check(amt);
}
