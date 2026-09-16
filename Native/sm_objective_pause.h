#pragma once
int sm_objective_pause_highlights(void);
#include "sm_bridge.h"
void sm_objective_pause_reset(void);
int sm_objective_pause_tick(void);
int sm_objective_pause_input(void);
int sm_objective_pause_buttons(void);
void sm_objective_pause_unpause(void);
/* Presentation copy only: 50-column original-asset tilemap for widescreen. */
const uint16_t *sm_objective_pause_wide(void);
SM_API int sm_objective_pause_state(int field);

void sm_objective_pause_load_assets(void);
