/* Presentation cues follow actual native scripts; no alternative encounter AI.
 * Phases: drain, healing, last charge, sacrifice, transfer, duel, death, escape. */
#include "sm_finale.h"
#include "sm_effects.h"
#include "sm_rush_runtime.h"
#include "ida_types.h"
#include "variables.h"
#include "enemy_types.h"
#include "funcs.h"
#include <string.h>
static struct {int phase,age,baby,bx,by,hits,shots,mx,my,shot_dir,sacrifice,fade,music,drain;} f;
static int textures_ready;
void sm_set_finale_textures(int ready){textures_ready=!!ready;}
int sm_finale_hyper_enabled(void){return sm_combat_effects && textures_ready && hyper_beam_flag;}
void sm_finale_reset(void){memset(&f,0,sizeof(f));f.baby=-1;}
int sm_finale_enabled(void){return sm_combat_effects && room_ptr==0xdd58;}
void sm_finale_phase(int phase){if(f.phase!=phase){f.phase=phase;f.age=0;if(phase==6)f.music=1;}}
void sm_finale_baby(unsigned index){f.baby=index;}
void sm_finale_hit(void){f.hits++;}
void sm_finale_shot(unsigned i){if(i<10){f.shots++;f.mx=projectile_x_pos[i];f.my=projectile_y_pos[i];f.shot_dir=projectile_dir[i];}}
void sm_finale_frame(void){
 if(game_state<8 || game_state>18){sm_finale_reset();return;}
 if(room_ptr!=0xdd58){f.phase=f.age=0;f.baby=-1;return;}
 if(game_state!=8)return;
 f.age++;
 if(f.phase==1){
  Enemy_MotherBrain *body=Get_MotherBrain(0),*head=Get_MotherBrain(0x40);
  int target=0;
  switch(body->mbn_var_A){
   case FUNC16(MotherBrain_DrainedByShitroid_1):target=64;break;
   case FUNC16(MotherBrain_DrainedByShitroid_2):target=96+head->mbn_var_06*60;break;
   case FUNC16(MotherBrain_DrainedByShitroid_3):target=480;break;
   case FUNC16(MotherBrain_DrainedByShitroid_4):target=560;break;
   case FUNC16(MotherBrain_DrainedByShitroid_5):target=640;break;
   case FUNC16(MotherBrain_DrainedByShitroid_6):target=740;break;
   case FUNC16(MotherBrain_DrainedByShitroid_7):target=800+body->mbn_var_37*24;break;
   default:if(body->mbn_var_1F)target=1024;break;
  }
  if(target>1024)target=1024;
  if(f.drain<target){f.drain+=3;if(f.drain>target)f.drain=target;}
 }
 if(f.baby>=0 && f.baby<0x800){EnemyData *e=gEnemyData(f.baby);f.bx=e->x_pos;f.by=e->y_pos;}
 if(f.phase==4 || f.phase==5)f.sacrifice++;
 if(f.phase==7 && Get_MotherBrain(0)->mbn_var_A==0xb1d5)f.fade++;
}
int sm_finale_music(void){
 if(!sm_combat_effects || game_state<8 || game_state>18)return 0;
 // Keep the same song/cursor across doors and pause until the ship cinematic.
 if(f.music && !sm_rush_active())return 2;
 if(room_ptr!=0xdd58)return 0;
 if(f.phase==4 || f.phase==5)return 1;
 if(f.phase==6 || f.phase==7)return 2;
 /* Keep the faded finale bus until Rush hands over to its result music.
  * Releasing it here would restart the ordinary combat theme for 20 frames. */
 if(f.phase==8 && sm_rush_active() && (sm_rush_info(0)==SM_RUSH_FIGHT || sm_rush_info(0)==SM_RUSH_END))return 2;
 return 0;
}
int sm_finale_gain(void){if(!sm_rush_active())return 32768;if(f.phase==8)return 0;int n=f.fade;return n>=120?0:(120-n)*32768/120;}
int sm_finale_state(int field){
 switch(field){case 55:return f.phase;case 56:return f.bx;case 57:return f.by;
 case 58:return f.age;case 59:return hyper_beam_flag;case 60:return f.hits;case 61:return f.shots;
 case 62:return Get_MotherBrain(0x40)->base.health;case 63:return f.sacrifice;case 64:return f.fade;
 case 65:return f.mx;case 66:return f.my;case 67:return f.shot_dir;
 case 68:return timer_status && area_index!=6 && CheckEventHappened(14);
 case 70:return f.drain;case 71:return Get_MotherBrain(0)->base.x_pos;
 case 72:return Get_MotherBrain(0)->base.y_pos;default:return 0;}
}
