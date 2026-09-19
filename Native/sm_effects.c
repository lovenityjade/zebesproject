#include "sm_finale.h"
#include "sm_effects.h"
#include "sm_relic.h"
#include "ida_types.h"
#include "variables.h"
#include "enemy_types.h"
#include "funcs.h"
#include <string.h>
static struct {int x,y,kind;} events[32];
static int event_count,charge_x,charge_y,charge_valid;
static unsigned totals[4];static int last_power,last_grapple,last_screw;
void sm_visual_reset(void){sm_finale_reset();memset(totals,0,sizeof(totals));last_power=last_grapple=last_screw=0;}
void sm_visual_end_frame(void){
  sm_finale_frame();
  if(game_state!=8)return;
  int power=(power_bomb_explosion_status&0x8000) && power_bomb_explosion_radius;
  int grapple=grapple_beam_function>=0xc51e && grapple_beam_function<0xc856 && grapple_beam_length>0;
  int screw=samus_contact_damage_index==3 && (samus_pose==0x81 || samus_pose==0x82);
  if(power&&!last_power)totals[1]++;if(grapple&&!last_grapple)totals[2]++;if(screw&&!last_screw)totals[3]++;
  last_power=power;last_grapple=grapple;last_screw=screw;
}
void sm_visual_charge(uint16_t x,uint16_t y){charge_x=x;charge_y=y;charge_valid=1;}
int sm_combat_effects;
void sm_set_combat_effects(int enabled) {sm_combat_effects=!!enabled;}
void sm_visual_begin_frame(void) {event_count=0;charge_valid=0;}
void sm_visual_sprite(uint16_t x,uint16_t y,uint16_t kind) {
  if(kind==128)totals[0]++;
  if(event_count<32) {events[event_count].x=x;events[event_count].y=y;events[event_count++].kind=kind;}
}
int sm_visual_eye(void) {
  if(game_state!=8)return 0;
  for(int i=0;i<6;i++)if(hdma_object_channels_bitmask[i] &&
      (hdma_object_pre_instruction_bank[i]&255)==0x88 &&
      (hdma_object_pre_instructions[i]==0xe9e6 || hdma_object_pre_instructions[i]==0xea3c || hdma_object_pre_instructions[i]==0xeacb))return 1;
  return 0;
}
int sm_visual_suit(void) {
  if(game_state!=8)return 0;
  for(int i=0;i<6;i++)if(hdma_object_channels_bitmask[i] &&
      (hdma_object_pre_instruction_bank[i]&255)==0x88) {
    if(hdma_object_pre_instructions[i]==0xe026)return 1;
    if(hdma_object_pre_instructions[i]==0xe05c)return 2;
  }
  return 0;
}
int sm_visual_motherbrain_beam(void) {
  if(game_state!=8 || room_ptr!=0xdd58)return 0;
  switch(Get_MotherBrain(0)->mbn_var_A) {
    case FUNC16(MotherBomb_FiringRainbowBeam_1_StartCharge):
    case FUNC16(MotherBomb_FiringRainbowBeam_2_RetractNeck):
    case FUNC16(MotherBomb_FiringRainbowBeam_3_Wait):
    case FUNC16(MotherBomb_FiringRainbowBeam_4_ExtendNeckDown):
    case FUNC16(MotherBomb_FiringRainbowBeam_5_StartFiring):return 1;
    case FUNC16(MotherBomb_FiringRainbowBeam_6_MoveSamusToWall):
    case FUNC16(MotherBomb_FiringRainbowBeam_7_DelayFrame):
    case FUNC16(MotherBomb_FiringRainbowBeam_8_StartDrainSamus):
    case FUNC16(MotherBomb_FiringRainbowBeam_9_DrainingSamus):return 2;
    case FUNC16(MotherBomb_FiringRainbowBeam_10_FinishFiringRainbow):return 3;
    default:return 0;
  }
}
int sm_visual_motherbrain_beam_mask(void) {
  // Automatic reserves freeze the same room in state 27. Keep the residual
  // beam color suppressed while they refill Samus after the discharge.
  if((game_state!=8 && game_state!=27) || room_ptr!=0xdd58)return 0;
  if(sm_visual_motherbrain_beam()>=2)return 1;
  if(sm_finale_enabled() && sm_finale_state(55)>=1)return 1;
  // The HDMA channel ends before the native color-math configuration is reset.
  // Keep its residual flat red backdrop out of the enhanced presentation while
  // Samus falls and the next scripted attack/baby entrance is prepared.
  switch(Get_MotherBrain(0)->mbn_var_A) {
    case FUNC16(MotherBomb_FiringRainbowBeam_11_LetSamusFall):
    case FUNC16(MotherBomb_FiringRainbowBeam_12_WaitForSamusHitGround):
    case FUNC16(MotherBomb_FiringRainbowBeam_13_LowerHead):
    case FUNC16(MotherBomb_FiringRainbowBeam_14_DecideNextAction):
    case FUNC16(MotherBrain_Phase2Cut_0):
    case FUNC16(MotherBrain_Phase2Cut_1):
    case FUNC16(MotherBrain_Phase2Cut_2):
    case FUNC16(MotherBrain_Phase2Cut_3):
    case FUNC16(MotherBrain_Phase2Cut_4):
    case FUNC16(MotherBrain_Phase2Cut_5):
    case FUNC16(nullsub_364):return 1;
    default:return 0;
  }
}
int sm_visual_state(int field,int index) {
  if(field==69)return index>=0 && index<10?projectile_dir[index]:0;
  if(field>=55 && field<=72)return sm_finale_state(field);
  int i=index;
  switch(field) {
    case 0:return samus_movement_type;
    case 1:return samus_anim_frame;
    case 2:return samus_y_radius;
    case 3:return samus_pose_x_dir;
    case 4:return flare_counter<0x8000?flare_counter:0;
    case 5:return i>=0&&i<10?projectile_bomb_instruction_ptr[i]:0;
    case 6:return i>=0&&i<10?projectile_x_pos[i]:0;
    case 7:return i>=0&&i<10?projectile_y_pos[i]:0;
    case 8:return i>=0&&i<10?projectile_type[i]:0;
    case 9:return power_bomb_explosion_radius;
    case 10:return power_bomb_pre_explosion_flash_radius;
    case 11:return power_bomb_explosion_x_pos;
    case 12:return power_bomb_explosion_y_pos;
    case 13:return power_bomb_explosion_status;
    case 14:return event_count;
    case 15:return i>=0&&i<event_count?events[i].x:0;
    case 16:return i>=0&&i<event_count?events[i].y:0;
    case 17:return i>=0&&i<event_count?events[i].kind:0;
    case 18:return samus_x_pos;
    case 19:return samus_y_pos;
    case 20:return equipped_beams;
    case 21:return samus_health;
    case 22:return charge_x;
    case 23:return charge_y;
    case 24:return charge_valid;
    case 25:return joypad1_lastkeys;
    case 26:return samus_input_handler;
    case 27:return samus_movement_handler;
    case 28:return time_is_frozen_flag;
    case 29:return button_config_shoot_x;
    case 30:return (int16_t)x_pos_of_start_of_grapple_beam;
    case 31:return (int16_t)y_pos_of_start_of_grapple_beam;
    case 32:return (int16_t)grapple_beam_end_x_pos;
    case 33:return (int16_t)grapple_beam_end_y_pos;
    case 34:return grapple_beam_function>=0xc51e && grapple_beam_function<0xc856 && grapple_beam_length>0;
    case 35:return samus_contact_damage_index==3 && (samus_pose==0x81 || samus_pose==0x82);
    case 36:return collected_items;
    case 37:return collected_beams;
    case 38:return sm_visual_eye();
    case 39:return enemy_data[1].x_pos;
    case 40:return enemy_data[1].y_pos;
    case 46:return sm_relic_escape_active();
    case 47:return samus_contact_damage_index==1 || samus_contact_damage_index==2;
    case 48:return button_config_run_b;
    case 49:return sm_visual_suit();
    case 50:return sm_visual_suit()?substate:0;
    case 51:return sm_visual_motherbrain_beam();
    case 52:return Get_MotherBrain(0x40)->base.x_pos+16;
    case 53:return Get_MotherBrain(0x40)->base.y_pos+4;
    case 54:return (fx_type==2 || fx_type==4) && !(lava_acid_y_pos&0x8000)?lava_acid_y_pos:32767;
    case 42:case 43:case 44:case 45:return totals[field-42];
    default:return 0;
  }
}

/* Footfall events follow the game's animation cadence. Classification only
 * reads native environment state; it never probes/writes collision scratch. */
void sm_visual_footstep(void) {
  if(game_state!=8 || time_is_frozen_flag || samus_movement_type!=1 || samus_y_speed)return;
  int feet=samus_y_pos+samus_y_radius;
  int water=sm_water_y(),style=0;
  if(water!=32767 && feet>water+8)style=2;             /* submerged silt */
  else if(fx_type==10 || (water!=32767 && feet>=water-4))style=1;
  else if(sm_heated_room())style=4;                   /* dry volcanic ash */
  else if(flag_samus_in_quicksand || area_index==4)style=3;
  else if(area_index==3 || area_index==5 || area_index==6)style=5;
  sm_visual_sprite(samus_x_pos+(samus_anim_frame<5?-4:4),feet,129+style);
}
