#pragma once
#include "sm_bridge.h"
enum {SM_VARIA_AMMO=1,SM_VARIA_HUD=2,SM_VARIA_RESERVES=4,SM_VARIA_MARKERS=8};
SM_API void sm_varia_ui_configure(unsigned mask);
/* 100 flags in SMItemAddresses order. NULL/0 restores historical counting. */
SM_API void sm_varia_ui_counted_configure(const uint8_t *counts,int count);
SM_API int sm_varia_ui_state(int field);
int sm_varia_ui_active(unsigned flag);
void sm_varia_ui_reset(void);
void sm_varia_ui_frame(void);
void sm_varia_ui_render(uint8_t *native_pixels);
int sm_varia_reserve_transfer(void);
int sm_varia_reserve_manual(void);

int sm_varia_region(void);

void sm_varia_ui_load_assets(void);
