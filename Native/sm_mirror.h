#pragma once
#include "sm_bridge.h"
/* Compatibility ABI for the excluded Mirror mode. Enabling it is rejected;
 * inactive calls preserve ordinary Vanilla/Randomizer behavior. */
SM_API int sm_mirror_configure(int slot,int enabled,const char *catalog);
void sm_mirror_reset(void);
void sm_mirror_activate(int slot);
void sm_mirror_apply(int randomized);
int sm_mirror_active(void);
int sm_mirror_door(uint32_t address);
void sm_mirror_capture(uint8_t *rom);
void sm_mirror_restore(uint8_t *rom);
void sm_mirror_apply_rom(uint8_t *rom);
uint32_t sm_mirror_item_address(int index,uint32_t canonical);
