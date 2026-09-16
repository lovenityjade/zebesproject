#pragma once
#include "sm_bridge.h"
#include "sm_indicators.h"
/* Four independent configurations: A/B/C and a reserved legacy slot.
 * The catalog hash binds numeric patch IDs to the exact compiled data. */
SM_API int sm_start_configure(int slot,int spawn,const uint8_t *patches,int count,const char *catalog);
SM_API int sm_start_configure_world(int slot,int spawn,const uint8_t *patches,int count,const char *catalog,const SmDoorIndicator *indicators,int indicator_count);
SM_API int sm_start_spawn(int slot);
SM_API int sm_start_configure_initial_doors(int slot,int spawn,const uint8_t *doors,int count);
void sm_start_reset(void);
int sm_start_activate(int slot,int enabled);
int sm_start_active_slot(void);
void sm_start_initialize_save(void);
void sm_start_new_game(void);
void sm_start_room(void);
void sm_start_door(void);
