#pragma once
#include "sm_bridge.h"
/* Immutable source variant IDs; pending slots commit only with a valid seed. */
SM_API int sm_animals_configure(int slot,int mode,const char *catalog);
SM_API const char *sm_animals_catalog_sha256(void);
SM_API int sm_animals_state(void);
void sm_animals_reset(void);
void sm_animals_activate(int slot);
void sm_animals_capture(uint8_t *rom);
void sm_animals_restore(uint8_t *rom);
void sm_animals_apply(uint8_t *rom,int randomized);
int sm_animals_room(uint32_t address);
int sm_animals_door(uint32_t address);
int sm_animals_escape_event(void);
int sm_animals_hostile(void);

void sm_animals_load_assets(void);
