#pragma once
#include "sm_bridge.h"
SM_API int sm_visual_state(int field,int index);
SM_API void sm_set_combat_effects(int enabled);
SM_API int sm_test_combat_equipment(void); /* private SMTests saves only */
extern int sm_combat_effects;
void sm_visual_begin_frame(void);
void sm_visual_footstep(void);
void sm_visual_sprite(uint16_t x,uint16_t y,uint16_t kind);

void sm_visual_charge(uint16_t x,uint16_t y);

int sm_visual_eye(void);
int sm_visual_suit(void); /* 0 none, 1 Varia, 2 Gravity; native HDMA lifecycle */
int sm_visual_motherbrain_beam(void); /* charge / strike / native taper */
int sm_visual_motherbrain_beam_mask(void); /* include native color-math recovery */
SM_API int sm_test_motherbrain_beam(int stage); /* copied SMTests saves only */
SM_API int sm_test_suit_pickup(int suit,int start); /* isolated SMTests only */

void sm_visual_reset(void);
void sm_visual_end_frame(void);
