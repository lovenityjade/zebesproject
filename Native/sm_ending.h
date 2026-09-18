#pragma once
#include "sm_seed.h"
/* In-memory preview: -1 keeps this save's rescue flag, 0/1 selects a variant. */
SM_API int sm_ending_preview_launch(int animals);
SM_API void sm_ending_preview_close(void);
SM_API int sm_ending_preview_active(void);
int sm_ending_preview_step(uint16_t buttons,uint8_t *pixels,int16_t *audio);
