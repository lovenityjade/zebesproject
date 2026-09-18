#pragma once
#include "sm_bridge.h"
/* Presentation only; no story triggers, save data, or game-state writes. */
void sm_dialogue_load_assets(void); /* after verified ROM load */
void sm_dialogue_unload(void);
SM_API int sm_dialogue_open(const char *text,int width); /* 256/400, ASCII, <=4096 bytes */
SM_API void sm_dialogue_tick(float seconds,int confirm_held);
SM_API void sm_dialogue_close(void); /* cancel */
/* 0 active, 1 page (zero-based), 2 pages, 3 fully revealed, 4 completed,
 * 5 width, 6 visible characters. */
SM_API int sm_dialogue_state(int field);
SM_API const uint8_t *sm_dialogue_pixels(void); /* BGRA, width x 52; no portrait */
