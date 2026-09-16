#pragma once
#include "sm_bridge.h"
void sm_route_open(const char *save);
int sm_route_save(void);
void sm_route_close(void);
void sm_route_session(void);
void sm_route_frame(void);
void sm_route_new_game(void);
void sm_route_finish(void);
void sm_route_slot_action(int action,int from,int to,const char *source_seed,const char *target_seed);
SM_API int sm_route_state(int field); /* 0 points,1 complete,2 partial,3 cursor */
SM_API const uint8_t *sm_route_pixels(int cursor); /* 528 x 320 BGRA */
