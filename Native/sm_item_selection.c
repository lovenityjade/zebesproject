#include "sm_item_selection.h"
#include "sm_bridge.h"
#include "ida_types.h"
#include "variables.h"
static uint16_t held;
static int direction;
void sm_item_selection_reset(void){held=0;direction=0;}
uint16_t sm_item_selection_input(uint16_t buttons){
  uint16_t extra=buttons&0x3000,pressed=extra&~held;
  held=extra;direction=0;
  if(game_state==8 && !sm_message_active() && !queued_message_box_index && !time_is_frozen_flag){
    if(extra!=0x3000)direction=(pressed&0x2000)?1:(pressed&0x1000)?-1:0;
  }
  return buttons&0xfff;
}
int sm_item_selection_direction(void){int result=direction;direction=0;return result;}
