#pragma once
#include "sm_bridge.h"
SM_API int sm_objective_event(unsigned event);
SM_API int sm_objective_enemy_count(int type,int field);
void sm_objective_event_mark(unsigned event);
uint16_t sm_objective_enemy_properties(unsigned bank,unsigned population,uint16_t original);
void sm_objective_enemy_death(unsigned index);
void sm_objective_events_frame(void);
