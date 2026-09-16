#pragma once
#include "sm_bridge.h"
enum { SM_ESCAPE_DISABLED_TOURIAN=1,SM_ESCAPE_REMOVE_ENEMIES=2,SM_ESCAPE_NOTHING_CRATERIA=4 };
/* Clock/common-behavior sub-contract. Routing and authored room data are
 * separate dependencies; no production manifest enables this on its own. */
typedef struct {
  uint32_t version,size;
  uint16_t flags,reserved,timer,half_timer;
  uint16_t timers[10],half_timers[10];
} SmEscapeClockPlan;
/* Ordered writes, not a permutation: VARIA repeats physical source doors. */
typedef struct {
  uint32_t version,size;
  uint16_t count,animals;
  uint8_t pairs[16][2];
} SmEscapeRoutingPlan;
SM_API const char *sm_escape_catalog_sha256(void);
SM_API int sm_escape_routing_configure(int slot,const SmEscapeRoutingPlan *plan,const char *catalog);
SM_API int sm_escape_clock_configure(int slot,const SmEscapeClockPlan *plan);
void sm_escape_reset(void);
void sm_escape_activate(int slot);
void sm_escape_apply(int randomized);
int sm_escape_enabled(void);
int sm_escape_configured(int slot);
int sm_escape_slot_flags(int slot);
int sm_escape_map_offset(int slot,int region);
int sm_escape_map_tile(int area,int byte,int mask);
int sm_escape_active(void);
int sm_escape_extended(void);
int sm_escape_nothing_allowed(void);
int sm_escape_trigger(void);
uint16_t sm_escape_timer(void);
int sm_escape_hyper(unsigned projectile);
void sm_escape_setup(void);
void sm_escape_room_setup(void);
void sm_escape_room_main(void);
void sm_escape_load_header(void);
int sm_escape_door(void);
void sm_escape_capture(uint8_t *rom);
void sm_escape_restore(uint8_t *rom);
void sm_escape_apply_rom(uint8_t *rom);

void sm_escape_load_assets(void);
