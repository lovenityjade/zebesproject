#include "sm_locale.h"
#include "sm_gameover.h"
#include "sm_map_browser.h"
#include "sm_cpu_infra.h"
#include "ida_types.h"
#include "variables.h"
#include <string.h>

int sm_gameover_state(int field){
  if(game_state!=kGameState_26_GameOverMenu)return field==0?0:-1;
  switch(field){case 0:return menu_index>=2;case 1:return file_select_map_area_index&1;case 2:return menu_index;default:return 0;}
}
const uint8_t *sm_gameover_labels(void){
  // Keep the original 48-row label region stable; the last 16 rows carry
  // the localized title for the Unreal presentation.
  static uint8_t pixels[128*64*4];memset(pixels,0,sizeof(pixels));
  if(!sm_gameover_state(0))return pixels;
  int selected=sm_gameover_state(1);
  sm_native_map_text(pixels,128,(128-8*sm_locale_length(sm_locale_text("CONTINUE")))/2,4,"CONTINUE",selected==0?0xe8f4ff:0x66788c);
  sm_native_map_text(pixels,128,(128-8*sm_locale_length(sm_locale_text("END")))/2,28,"END",selected==1?0xe8f4ff:0x66788c);
  if(sm_locale_get())sm_native_map_text(pixels,128,12,52,"FIN DE PARTIE",0xe8f4ff);
  return pixels;
}
