#pragma once
#include "sm_bridge.h"
/* Versioned 32-endpoint area domain. Never reinterpret the boss domain. */
SM_API const char *sm_areas_catalog_sha256(void);
SM_API int sm_areas_configure(int slot,const uint8_t *destinations,int count,const char *catalog);
int sm_areas_configured(int slot);
int sm_areas_destination(int index);
void sm_areas_reset(void);
void sm_areas_activate(int slot);
void sm_areas_capture(uint8_t *rom);
void sm_areas_restore(uint8_t *rom);
void sm_areas_apply_base(uint8_t *rom,int randomized);
void sm_areas_apply_routing(uint8_t *rom);
int sm_areas_door(void);
void sm_areas_room(void);
void sm_areas_refill(void);
int sm_areas_test_destination(int index,int *room,int *door,int *x,int *y);

void sm_areas_load_assets(void);
