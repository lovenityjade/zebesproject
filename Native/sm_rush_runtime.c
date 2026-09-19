#include "sm_rush_runtime.h"
#include "sm_locale.h"
#include "sm_dialogue.h"
#include "sm_boss_rush.h"
#include "sm_ending.h"
#include "sm_credits.h"
#include "sm_seed.h"
#include "sm_scene.h"
#include "sm_walljump.h"
#include "sm_spacejump.h"
#include "sm_item_selection.h"
#include "sm_map_browser.h"
#include "sm_cinematics.h"
#include "sm_soundtrack.h"
#include "sm_rtl.h"
#include "sm_cpu_infra.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include "enemy_types.h"
#include <string.h>
#include <stdlib.h>

void sm_rush_snapshot(int operation); /* 0 capture, 1 restore retained, 2 free */
static SmBossRush run;
static int stage,request=-1,practice,cheated,first,settle,won,pose_saved,mb_done,mb_final,hp_seen,deaths,continues;
static uint8_t pose[0x40c],damage_carry[32];
static uint16_t damage_enemy[32];
static uint32_t recoil_carry;
static int croco_origin;
typedef struct {uint16_t room,door,x,y,items,beams,energy,missiles,supers;uint8_t area,bit;} Arena;
/* Genuine incoming doors; no synthetic exit records. Charge is the renewable
 * damage source where missile capacity alone cannot safely cover the fight. */
static const Arena arenas[10]={
 {0x9804,0x8bc2,64,160,0x1004,0,99,10,0,0,4},
 {0x9dc7,0x8e3e,128,640,0,0x1000,199,10,0,1,2},
 {0xa59f,0x91b6,64,380,0x100,0x1004,199,10,5,1,1},
 {0xa98d,0x93d2,864,128,1,0x1006,199,15,5,2,2},
 {0xcd13,0xa2ac,64,160,0,0x1007,299,25,0,3,1},
 {0xd95e,0xa774,64,160,0,0x1007,299,30,0,4,2},
 {0xda60,0xa840,448,128,0x20,0x1007,399,60,0,4,1},
 {0xb283,0x983a,64,128,1,0x100f,399,15,5,2,4},
 {0xb32e,0x98ca,192,160,1,0x100f,499,0,30,2,1},
 {0xdd58,0xaac8,220,144,1,0x100f,699,40,0,5,1},
};
int sm_rush_active(void){return stage!=SM_RUSH_OFF;}
int sm_rush_info(int field){switch(field){case 0:return stage;case 1:return run.encounter;case 2:return run.difficulty;case 3:return practice;case 4:return cheated;case 5:return first;case 6:return (int)run.frames;case 7:return run.mother_brain_sequence;case 8:return deaths;case 9:return continues;default:return 0;}}
const char *sm_rush_name(void){return sm_boss_rush_encounter(run.encounter)->name;}
uint64_t sm_rush_split(int encounter){return encounter>=0&&encounter<10?run.splits[encounter]:0;}
int sm_rush_practice(int enabled){if(enabled>=0){practice=!!enabled;if(practice&&stage)cheated=1;}return practice;}
int sm_rush_request(int difficulty){
 if(stage || request>=0 || !g_snes || !sm_boss_rush_rules(difficulty) || sm_seed_active() ||
    sm_ending_preview_active() || sm_credits_state(0) || game_state!=2 || game_options_screen_index!=3)return 0;
 request=difficulty;return 1;
}
void sm_rush_close(void){
 if(!stage){request=-1;return;}
 sm_rush_snapshot(1);sm_rush_snapshot(2);stage=0;request=-1;pose_saved=0;
 sm_soundtrack_rush_status(0); /* End this attempt's external music cursor. */
 sm_scene_capture_metadata=0;sm_scene_reset();sm_cinema_begin(0);
 sm_walljump_reset();sm_spacejump_reset();sm_item_selection_reset();
 joypad1_lastkeys=joypad1_newkeys=0;
}
static void resources(void){
 samus_missiles=samus_max_missiles;samus_super_missiles=samus_max_super_missiles;
 samus_power_bombs=samus_max_power_bombs;
 if(!run.mother_brain_sequence){samus_health=samus_max_health;samus_invincibility_timer=2;samus_knockback_timer=0;}
}
static void load(void){
 sm_rush_snapshot(1); /* Reset all native AI/PLM/HDMA state between arenas. */
 if(sm_soundtrack_rush_status(0))sm_soundtrack_spc_command(0);
 const Arena *a=&arenas[run.encounter];
 memset(events_that_happened,0,8);memset(boss_bits_for_area,0,8);
 memset(item_bit_array,0,64);memset(opened_door_bit_array,0,64);
 if(run.encounter==SM_RUSH_BOMB_TORIZO)item_bit_array[0]|=0x80; /* Bomb, collection bit 7. */
 events_that_happened[0]=1; /* Awake Zebes, living bosses. */
 /* Upgrade before entering the arena, retaining each suit thereafter. */
 equipped_items=collected_items=a->items|(run.encounter>=SM_RUSH_CROCOMIRE?1:0)|(run.encounter>=SM_RUSH_BOTWOON?0x20:0)|(run.encounter>=SM_RUSH_DRAYGON?0x4004:0);collected_beams=a->beams;
 equipped_beams=collected_beams;
 /* Plasma and Spazer stay owned, but must never be equipped together. */
 if(equipped_beams&8)equipped_beams&=~4;
 samus_health=samus_max_health=sm_boss_rush_starting_energy(run.difficulty,a->energy);
 unsigned scale=run.difficulty==SM_RUSH_MEDIUM?1:2;
 samus_missiles=samus_max_missiles=a->missiles*scale;
 samus_super_missiles=samus_max_super_missiles=a->supers*scale;
 samus_power_bombs=samus_max_power_bombs=run.encounter>=SM_RUSH_DRAYGON?5*scale:0;
 samus_reserve_health=samus_max_reserve_health=reserve_health_mode=0;
 area_index=a->area;load_station_index=0;loading_game_state=5;game_state=6;
 coroutine_state_0=coroutine_state_1=coroutine_state_2=coroutine_state_3=coroutine_state_4=0;
 timer_status=0;game_time_frames=game_time_seconds=game_time_minutes=game_time_hours=0;
 stage=SM_RUSH_LOADING;settle=won=pose_saved=mb_done=mb_final=hp_seen=0;
 recoil_carry=0;
 memset(damage_carry,0,sizeof(damage_carry));memset(damage_enemy,0,sizeof(damage_enemy));
 sm_scene_reset();sm_scene_capture_metadata=1;sm_cinema_begin(0);
 sm_walljump_reset();sm_spacejump_reset();sm_item_selection_reset();
}
void sm_rush_load_room(void){
 if(stage!=SM_RUSH_LOADING)return;
 const Arena *a=&arenas[run.encounter];
 room_ptr=a->room;door_def_ptr=a->door;area_index=a->area;LoadMirrorOfExploredMapTiles();
 layer1_x_pos=bg1_x_offset=(a->x/256)*256;layer1_y_pos=bg1_y_offset=(a->y/256)*256;
 samus_x_pos=samus_prev_x_pos=a->x;samus_y_pos=samus_prev_y_pos=a->y;
}
int sm_rush_command(int command){
 if(command==0 && stage==SM_RUSH_OUT){load();return 1;}
 if(command==1 && stage==SM_RUSH_READY){
  stage=SM_RUSH_FIGHT;first=0;hp_seen=enemy_data[0].health!=0;sm_boss_rush_arena_ready(&run,run.encounter);
  /* Skip the load-from-save fanfare: controls begin with the VR reveal. */
  frame_handler_alfa=FUNC16(Samus_FrameHandlerAlfa_Func11);
  frame_handler_beta=FUNC16(Samus_FrameHandlerBeta_Func17);
  samus_input_handler=FUNC16(Samus_InputHandler_E913);
  samus_draw_handler=FUNC16(SamusDrawHandler_Default);
  samus_pose=arenas[run.encounter].door==0xaac8 || run.encounter==SM_RUSH_DRAYGON || run.encounter==SM_RUSH_RIDLEY?kPose_02_FaceL_Normal:kPose_01_FaceR_Normal;
  SamusFunc_F433();Samus_SetAnimationFrameIfPoseChanged();substate=0;
  /* StartGameplay mutes SFX until its save-load fanfare PLM finishes. Rush
   * reveals directly into combat, so shots must be audible immediately. */
  debug_disable_sounds=0;
  PlayRoomMusicTrackAfterAFrames(1);
  joypad1_lastkeys=joypad1_newkeys=0;return 1;
 }
 if(command==2 && stage==SM_RUSH_DEATH){stage=SM_RUSH_GAMEOVER;return 1;}
 if(command==3 && stage==SM_RUSH_END){stage=SM_RUSH_RESULTS;return 1;}
 if(command==4 && stage){sm_rush_close();return 1;}
 if(command==5 && stage==SM_RUSH_FIGHT && practice){cheated=1;won=1;return 1;}
 if(command==6 && stage==SM_RUSH_GAMEOVER){
  ++continues;run.state=SM_RUSH_TRANSITION;run.mother_brain_sequence=0;
  stage=SM_RUSH_OUT;first=1;return 1; /* Preserve failed time and split. */
 }
 if(command==7 && (stage==SM_RUSH_GAMEOVER || stage==SM_RUSH_RESULTS || stage==SM_RUSH_FIGHT)){
  int d=run.difficulty;sm_boss_rush_reset(&run);sm_boss_rush_begin(&run,d,0x5a454245);
  deaths=continues=0;cheated=practice;stage=SM_RUSH_OUT;first=1;return 1;
 }
 if(command==8 && stage){
  sm_rush_close();
  DisableHdmaObjects();WaitUntilEndOfVblankAndClearHdma();DisableIrqInterrupts();
  for(int i=656;i>=0;i-=2)*(uint16*)((uint8*)&cinematic_var5+i)=0;
  game_state=1;cinematic_function=0x9b68; /* Native title initialization. */
  return 1;
 }
 return 0;
}
int sm_rush_before(uint16_t *buttons){
 if(stage==SM_RUSH_LOADING){*buttons=0;return 0;}
 if(stage==SM_RUSH_FIGHT){
  if(practice)resources();
  /* Native Start map and room exits are not part of a timed boss encounter. */
  *buttons&=~8u;
  return 0;
 }
 return stage!=0;
}
static int defeated(void){
 if(run.encounter==SM_RUSH_CROCOMIRE)return Get_Crocomire(0)->crocom_var_A!=0;
 if(run.encounter==SM_RUSH_MOTHER_BRAIN)return mb_done || (mb_final && !enemy_data[1].health);
 return hp_seen && !enemy_data[0].health;
}
void sm_rush_draw_pose(void){
 if(!stage || pose_saved)return;
 if(!((stage==SM_RUSH_LOADING && game_state>=7) || (stage==SM_RUSH_FIGHT && (won || defeated() || (int16_t)samus_health<=0))))return;
 memcpy(pose,g_ram+0xa00,sizeof(pose));pose_saved=1;
 MakeSamusFaceForward();samus_draw_handler=FUNC16(SamusDisplayHandler_SamusReceivedFatal);
 samus_invincibility_timer=samus_knockback_timer=0;
}
void sm_rush_after(void){
 if(pose_saved){memcpy(g_ram+0xa00,pose,sizeof(pose));pose_saved=0;}
 if(request>=0 && !stage){
  int d=request;request=-1;sm_rush_snapshot(0);sm_boss_rush_reset(&run);
  sm_boss_rush_begin(&run,d,0x5a454245);stage=SM_RUSH_OUT;first=1;cheated=practice;deaths=continues=0;return;
 }
 if(stage==SM_RUSH_LOADING){
  if(game_state==8 && sm_brightness()==15 && !coroutine_state_0 && !coroutine_state_1 && ++settle>=2){stage=SM_RUSH_READY;croco_origin=enemy_data[0].x_pos;}
  return;
 }
 if(stage!=SM_RUSH_FIGHT)return;
 sm_boss_rush_tick(&run,game_state==8);
 if(game_state==19 || (int16_t)samus_health<=0){++deaths;sm_boss_rush_fail(&run);stage=SM_RUSH_DEATH;return;}
 /* Crocomire has no lethal HP threshold: its first acid-fall state is
  * the positional defeat. MB must reach the post-baby final phase first. */
 if(run.encounter!=SM_RUSH_MOTHER_BRAIN && enemy_data[0].health)hp_seen=1;
 if(defeated() || won){
  sm_boss_rush_complete_encounter(&run,run.encounter,1);
  stage=run.state==SM_RUSH_FINISHED?SM_RUSH_END:SM_RUSH_OUT;
 }

}
uint16_t sm_rush_damage(uint16_t amount){
 if(stage==SM_RUSH_LOADING)return 0;
 if(stage!=SM_RUSH_FIGHT)return amount;
 if(practice || sm_rush_mb_visual())return 0;
 uint32_t d=sm_boss_rush_player_damage(&run,amount,samus_health,SM_RUSH_HIT);
 return d>samus_health?samus_health:d;
}
uint32_t sm_rush_periodic(uint32_t amount){
 if(!stage)return amount;
 if(stage!=SM_RUSH_FIGHT || practice || sm_rush_mb_visual())return 0;
 if(!amount)return 0;
 if(sm_boss_rush_rules(run.difficulty)->one_hit_ko)return (uint32_t)samus_health<<16 | samus_subunit_health;
 return amount*sm_boss_rush_rules(run.difficulty)->received_damage;
}
uint32_t sm_rush_enemy_damage(uint32_t amount){
 if(!stage || !amount || run.encounter==SM_RUSH_CROCOMIRE)return amount;
 EnemyData *e=gEnemyData(cur_enemy_index);
 /* Only the primary health pool scales. Adds, detached parts, turrets and
  * the scripted baby retain their native HP. MB's brain is slot 1. */
 int boss=cur_enemy_index==(run.encounter==SM_RUSH_MOTHER_BRAIN?64:0);
 if(!boss)return amount;
 const SmBossRushRules *r=sm_boss_rush_rules(run.difficulty);int i=(cur_enemy_index>>6)&31;
 if(damage_enemy[i]!=e->enemy_ptr){damage_carry[i]=0;damage_enemy[i]=e->enemy_ptr;}
 uint32_t total=amount*r->hp_denominator+damage_carry[i];
 damage_carry[i]=total%r->hp_numerator;return total/r->hp_numerator;
}
/* Crocomire is defeated by distance, not HP. Inverse HP scaling makes each
 * successful recoil worth 4/3, 1, or 1/2 of its native distance. Preserve the
 * subpixel remainder, native hit reaction, weapon strengths and collision. */
uint32_t sm_rush_crocomire_recoil(uint32_t distance){
 if(stage!=SM_RUSH_FIGHT || run.encounter!=SM_RUSH_CROCOMIRE || Get_Crocomire(0)->crocom_var_A)return distance;
 const SmBossRushRules *r=sm_boss_rush_rules(run.difficulty);
 uint64_t total=(uint64_t)distance*r->hp_denominator+recoil_carry;
 recoil_carry=total%r->hp_numerator;
 return (uint32_t)(total/r->hp_numerator);
}
int sm_rush_drop(int drop){return stage?sm_boss_rush_drop(&run,drop):drop;}
int sm_rush_pickup(int amount){return stage?sm_boss_rush_pickup_quantity(run.difficulty,amount):amount;}
void sm_rush_mb_sequence(int enabled){if(stage==SM_RUSH_FIGHT){sm_boss_rush_mother_brain_sequence(&run,enabled);if(!enabled)mb_final=1;}}
int sm_rush_mb_visual(void){return stage==SM_RUSH_FIGHT && run.mother_brain_sequence && (practice || run.difficulty==SM_RUSH_HARDCORE);}
int sm_rush_mb_finish(void){if(stage && run.encounter==SM_RUSH_MOTHER_BRAIN){mb_done=1;return 1;}return 0;}
int sm_rush_block_doors(void){return stage==SM_RUSH_FIGHT;}

extern uint8_t sm_wide_hud[];
/* Same 2bpp energy-tank tiles as Samus, recolored coral. Ten tanks show
 * tenths of the current boss phase; the number below is its exact effective HP. */
static void boss_hud(uint8_t *out,int width,int opaque){
 int left=width-48,clear=width==400?328:208;
 for(int y=0;y<32;y++)for(int x=clear;x<width;x++){
  uint8_t *p=out+(y*width+x)*4;memset(p,0,4);p[3]=opaque?255:0;
 }
 int phase=run.encounter==SM_RUSH_MOTHER_BRAIN?Get_MotherBrain(0)->mbn_var_00:0;
 if(phase>2)phase=2;
 const SmBossRushRules *r=sm_boss_rush_rules(run.difficulty);
 unsigned maximum=sm_boss_rush_hp(run.difficulty,run.encounter,phase);
 unsigned raw=enemy_data[run.encounter==SM_RUSH_MOTHER_BRAIN?1:0].health;
 unsigned current=(raw*r->hp_numerator+r->hp_denominator-1)/r->hp_denominator;
 if(run.encounter==SM_RUSH_CROCOMIRE){
  maximum=croco_origin<1536?1536-croco_origin:1;
  current=enemy_data[0].x_pos<1536?1536-enemy_data[0].x_pos:0;
 }
 if(current>maximum)current=maximum;
 unsigned tanks=maximum?(current*10+maximum-1)/maximum:0;
 for(int n=0;n<10;n++){
  const uint8_t *tile=RomFixedPtr(0x9ab200)+(n<(int)tanks?0x31:0x30)*16;
  for(int y=0;y<8;y++)for(int x=0;x<8;x++){
   unsigned color=((tile[y*2]>>(7-x))&1)|(((tile[y*2+1]>>(7-x))&1)<<1);
   if(!color)continue;
   const uint32_t colors[]={0,0x50252a,0xff7048,0xf4d5bd};
   uint32_t rgb=colors[color];uint8_t *p=out+(((n/5)*8+y)*width+left+(n%5)*8+x)*4;
   p[0]=rgb;p[1]=rgb>>8;p[2]=rgb>>16;p[3]=255;
  }
 }
 char text[16];
 if(run.encounter==SM_RUSH_CROCOMIRE)strcpy(text,"PUSH");
 else snprintf(text,sizeof(text),"%05u",current);
 sm_native_map_text(out,width,left,22,text,0xff9470);
}
void sm_rush_render(uint8_t *pixels){
 if(stage!=SM_RUSH_FIGHT && stage!=SM_RUSH_READY)return;
 boss_hud(pixels,256,1);boss_hud((uint8_t*)sm_ui_overlay(),256,0);boss_hud(sm_wide_hud,400,0);
 char text[48];unsigned f=(unsigned)run.frames;
 snprintf(text,sizeof(text),"%02u:%02u.%02u",f/3600,f/60%60,f%60*100/60);
 sm_native_map_text(pixels,256,8,33,text,0x80e8ff);
 sm_native_map_text((uint8_t*)sm_ui_overlay(),256,8,33,text,0x80e8ff);
 sm_native_map_text(sm_wide_hud,400,8,33,text,0x80e8ff);
 if(cheated){
  sm_native_map_text(pixels,256,8,42,sm_locale_text("PRACTICE"),0xffbb66);
  sm_native_map_text((uint8_t*)sm_ui_overlay(),256,8,42,sm_locale_text("PRACTICE"),0xffbb66);
  sm_native_map_text(sm_wide_hud,400,8,42,sm_locale_text("PRACTICE"),0xffbb66);
 }
}

/* ROM-derived lettering for both 16:9 simulation panels. No external font. */
const uint8_t *sm_rush_screen(int selection,int paused){
 static uint8_t pixels[256*224*4];memset(pixels,0,sizeof(pixels));
 if(stage!=SM_RUSH_RESULTS)sm_native_frame(pixels,256,128);
 char text[80];unsigned f=(unsigned)run.frames;
 #define LABEL(x,y,s,c) sm_native_map_text(pixels,256,x,y,s,c)
 if(stage==SM_RUSH_RESULTS){
  snprintf(text,sizeof(text),sm_locale_text("TIME %02u:%02u.%02u"),f/3600,f/60%60,f%60*100/60);LABEL(0,0,text,0xe4fcff);
  LABEL(0,12,sm_boss_rush_rules(run.difficulty)->name,0x80d8ff);
  snprintf(text,sizeof(text),sm_locale_text("DEATHS %d   CONTINUES %d"),deaths,continues);LABEL(0,24,text,0x9ec4da);
  LABEL(0,36,cheated?"PRACTICE / UNRANKED":"10 / 10 BOSSES",0x9ec4da);
  static const char *names[]={"BT","SS","KR","CR","PH","BW","DR","GT","RI","MB"};
  for(int i=0;i<10;i++){
   unsigned t=(unsigned)run.splits[i];snprintf(text,sizeof(text),"%s %02u:%02u.%02u",names[i],t/3600,t/60%60,t%60*100/60);
   LABEL((i/5)*128,52+(i%5)*11,text,0xc0eaff);
  }
  LABEL(0,116,"A / ENTER - END",0xe4fcff);
 }else{
  const char *title=sm_locale_text(paused?"PAUSE":"SIMULATION FAILED"),*cursor=title;int columns=0;
  while(*cursor){sm_locale_next(&cursor);columns++;}
  LABEL((256-columns*8)/2,12,title,0xffe080);
  const char *options[]={"CONTINUE","RETRY","END"};
  for(int i=0;i<3;i++){
   if(i==selection)sm_native_menu_cursor(pixels,256,128,24,28+i*18);
   LABEL(48,32+i*18,options[i],i==selection?0xffe080:0xe4fcff);
  }
  const char *hint=selection==0?(paused?"RESUME THE FIGHT":"SAME BOSS. KEEP THE TIMER."):selection==1?"FIRST BOSS. RESET THE TIMER.":"RETURN TO TITLE.";
  /* Keep full existing translations; fit hints by wrapping native glyphs. */
  const char *label=sm_locale_text(hint);char row[128];int n=0,cols=0,last=-1,y=88;
  while(*label && y<108){
   const char *start=label;int cp=sm_locale_next(&label);int bytes=(int)(label-start);
   if(cp==' ')last=n;
   if(n+bytes>120)break;
   memcpy(row+n,start,bytes);n+=bytes;cols++;
   if(cols==28 || !*label){
    if(*label && last>0){label-=n-last-1;n=last;}
    row[n]=0;LABEL(16,y,row,0xc0d8f0);y+=10;n=cols=0;last=-1;
   }
  }
  snprintf(text,sizeof(text),sm_locale_text("TIME %02u:%02u.%02u"),f/3600,f/60%60,f%60*100/60);LABEL(16,112,text,0x80d8ff);
 }
 #undef LABEL
 return pixels;
}
