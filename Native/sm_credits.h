#pragma once
#include "sm_seed.h"
SM_API int sm_credits_launch(void);
SM_API void sm_credits_close(void);
SM_API int sm_credits_state(int field);
void sm_credits_load_assets(void);
void sm_credits_reset(void);
void sm_credits_start_ending(void);
int sm_credits_native_tick(void);
int sm_credits_preview_step(uint16_t buttons,uint8_t *pixels,int16_t *audio);
void sm_credits_render(uint8_t *pixels);
