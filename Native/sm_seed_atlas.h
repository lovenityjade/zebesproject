#pragma once
#include "sm_bridge.h"
/* Presentation coordinates only. Logic and SRAM always retain ROM coordinates. */
SM_API int sm_seed_atlas_available(void);
SM_API int sm_seed_atlas_active(void);
SM_API int sm_seed_atlas_location(int area,int x,int y,int *out_x,int *out_y);
SM_API int sm_seed_atlas_cell_size(void);
SM_API int sm_seed_atlas_focus(int area,int x,int y);
int sm_seed_atlas_project(int area,int map_x,int map_y,int width,int *x,int *y);
void sm_seed_atlas_reset(void);
int sm_seed_atlas_tick(void);
void sm_seed_atlas_render(uint8_t *pixels);
void sm_seed_atlas_cursor(uint8_t *pixels);
int sm_seed_atlas_local(int area,int x,int y);
void sm_seed_atlas_minimap(uint8_t *pixels);
void sm_seed_atlas_focus_area(int area);
