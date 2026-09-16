#pragma once
#include "sm_seed.h"
/* Read-only presentation metadata. Texture rows: geometry, color, direction. */
SM_API const float *sm_cinema_lights(void);
SM_API int sm_cinema_state(int field);
void sm_cinema_begin(int scene);
void sm_cinema_sprite(int slot,int definition);
int sm_cinema_sprite_gui(int slot);
void sm_cinema_frame(void);
