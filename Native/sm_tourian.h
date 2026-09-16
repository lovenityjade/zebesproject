#pragma once
#include "sm_bridge.h"
SM_API int sm_tourian_fast(void);
void sm_tourian_capture(uint8_t *rom);
void sm_tourian_restore(uint8_t *rom);
void sm_tourian_apply(uint8_t *rom);
int sm_tourian_door(void);
void sm_tourian_room(void);
void sm_tourian_frame(void);
void sm_tourian_hyper_start(void);
void sm_tourian_hyper_end(void);
