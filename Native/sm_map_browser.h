#pragma once
#include "sm_bridge.h"
void sm_map_browser_reset(void);
int sm_map_browser_tick(void);
int sm_map_browser_overview(void);
int sm_map_browser_area_visible(int area);
int sm_map_browser_view_area(void);
void sm_map_browser_refresh(void);
void sm_map_browser_render(uint8_t *pixels);
SM_API int sm_map_browser_state(int field);
void sm_native_map_text(uint8_t *out,int width,int x,int y,const char *text,uint32_t color);

void sm_native_text_height(uint8_t *out,int width,int height,int x,int y,const char *text,uint32_t color);
