#include "sm_ending.h"
#include "sm_bridge.h"
#include "sm_credits.h"
#include "sm_cinematics.h"
#include "sm_scene.h"
#include "sm_rtl.h"
#include "sm_cpu_infra.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
extern uint8_t sm_wide_pixels[],sm_wide_hud[];
void sm_ending_runtime_capture(void);
void sm_ending_runtime_restore(void);
void sm_ending_runtime_frame(void);
static int active,previous_buttons,saved_scene;
static uint8_t saved_pixels[256*240*4*3+400*240*4*2];
static void copy_pixels(int restore) {
  uint8_t *p[]={(uint8_t*)sm_pixels(),sm_scene_pixels,(uint8_t*)sm_ui_overlay(),sm_wide_pixels,sm_wide_hud};
  const size_t sizes[]={256*240*4,256*240*4,256*240*4,400*240*4,400*240*4};
  size_t offset=0;
  for(int i=0;i<5;i++){if(restore)memcpy(p[i],saved_pixels+offset,sizes[i]);else memcpy(saved_pixels+offset,p[i],sizes[i]);offset+=sizes[i];}
}
int sm_ending_preview_active(void){return active;}
int sm_ending_preview_launch(int animals) {
  if(animals < -1 || animals>1 || !g_snes || active || sm_credits_state(0) ||
     (game_state!=8 && game_state!=15) || sm_message_active() || queued_message_box_index)return 0;
  sm_ending_runtime_capture();copy_pixels(0);saved_scene=sm_cinema_state(0);
  active=1;previous_buttons=8;
  if(animals>=0){if(animals)SetEventHappened(15);else ClearEventHappened(15);}
  // Enter the same post-takeoff cinematic, after the native HDMA cleanup.
  DisableHdmaObjects();WaitUntilEndOfVblankAndClearHdma();DisableIrqInterrupts();
  fx_layer_blending_config_a=next_gameplay_CGWSEL=next_gameplay_CGADSUB=0;
  timer_status=power_bomb_explosion_status=0;
  coroutine_state_0=coroutine_state_1=coroutine_state_2=coroutine_state_3=coroutine_state_4=0;
  game_state=39;cinematic_function=FUNC16(CinematicFunctionEscapeFromCebes);
  reg_INIDISP=0x80;
  sm_cinema_begin(0);
  return 1;
}
void sm_ending_preview_close(void) {
  if(!active)return;
  sm_ending_runtime_restore();active=0;
  sm_scene_reset();sm_cinema_begin(saved_scene);copy_pixels(1);
}
int sm_ending_preview_step(uint16_t buttons,uint8_t *pixels,int16_t *audio) {
  if(!active)return 0;
  if((buttons&8) && !(previous_buttons&8)){
    sm_ending_preview_close();memset(audio,0,736*2*sizeof(*audio));return 1;
  }
  previous_buttons=buttons;
  // Restore before credits can mark the real run finished or unlock anything.
  if(cinematic_function==FUNC16(CinematicFunction_Intro_Func126)){
    sm_ending_preview_close();sm_credits_launch();
    memset(audio,0,736*2*sizeof(*audio));return 1;
  }
  sm_scene_active=0;sm_ending_runtime_frame();
  RtlRenderAudio(audio,736,2);
  for(int i=3;i<256*240*4;i+=4)pixels[i]=255;
  sm_scene_finish();sm_cinema_frame();
  return 1;
}
