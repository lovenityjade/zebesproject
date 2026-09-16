#pragma once
#include "sm_bridge.h"
/* Optional load-menu travel. Destinations come only from this slot's SRAM. */
SM_API void sm_travel_configure(int slot,int enabled);
SM_API int sm_travel_enabled(void);
SM_API int sm_travel_station_mask(int area);
void sm_travel_reset(void);
int sm_travel_area_input(void);
int sm_travel_room_input(void);
void sm_travel_draw_stations(void);
void sm_travel_render(uint8_t *pixels);
