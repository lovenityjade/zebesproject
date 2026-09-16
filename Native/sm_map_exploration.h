#pragma once
#include "sm_bridge.h"
/* Native counters derived from actual exploration, never map-station knowledge.
 * region -1 = total, 0..11 = VARIA graph area; field 0 explored, 1 available.
 * Full vanilla/area layouts only; modified-layout modes remain generation-gated. */
SM_API int sm_map_exploration_value(int region,int field);
int sm_map_exploration_region(void);
/* Descriptor validation before a pending world is committed. */
int sm_map_exploration_total(int area_layout,int region);
void sm_map_exploration_mark(unsigned byte,unsigned mask);
void sm_map_exploration_portal(int source,int destination);
void sm_map_exploration_capture(uint8_t *rom);
void sm_map_exploration_apply(uint8_t *rom,int randomized);
void sm_map_exploration_pack(void);
void sm_map_exploration_unpack(void);
