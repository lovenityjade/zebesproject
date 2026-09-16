#pragma once
#include "sm_bridge.h"
typedef struct {
  uint32_t version,size;
  uint16_t regions;
  uint8_t destinations[40],checks[100];
  uint16_t reserved;
} SmMinimizerPlan;
SM_API const char *sm_minimizer_catalog_sha256(void);
SM_API int sm_minimizer_configure(int slot,const SmMinimizerPlan *plan,const char *catalog);
SM_API int sm_minimizer_active(void);
SM_API int sm_minimizer_destination(int index);
int sm_minimizer_configured(int slot);
int sm_minimizer_region(int slot,int region);
int sm_minimizer_check(int index);
void sm_minimizer_reset(void);
void sm_minimizer_activate(int slot);
void sm_minimizer_capture(uint8_t *rom);
void sm_minimizer_restore(uint8_t *rom);
void sm_minimizer_apply(int randomized);
void sm_minimizer_apply_rom(uint8_t *rom);
int sm_minimizer_door(void);
void sm_minimizer_room(void);
int sm_minimizer_room_area(int endpoint);
int sm_minimizer_map_total(int slot,int region);
int sm_minimizer_enemy_total(int slot,int family);
int sm_minimizer_map_tile(int region,int area,int byte,int mask);
int sm_minimizer_slot_check(int slot,int index);
