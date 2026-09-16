#pragma once
#include "sm_bridge.h"
SM_API const char *sm_doors_catalog_sha256(void);
SM_API int sm_doors_configure(int slot,const uint8_t *colors,int count,const char *catalog);
#define SM_DOOR_COLOR_COUNT 58
int sm_doors_configured(int slot);
int sm_doors_map_color(int index);
void sm_doors_reset(void);
void sm_doors_initialize_save(void);
void sm_doors_activate(int slot);
void sm_doors_capture(uint8_t *rom);
void sm_doors_restore(uint8_t *rom);
void sm_doors_apply(uint8_t *rom,int randomized);
int sm_doors_preinstr(unsigned address,unsigned k);
int sm_doors_plm_direction(unsigned plm,int indicators_only);
