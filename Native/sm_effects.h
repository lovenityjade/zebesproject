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

void sm_visual_reset(void);
void sm_visual_end_frame(void);
