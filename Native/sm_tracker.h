#pragma once
#include "sm_bridge.h"
#include "sm_objectives.h"
/* Game-thread snapshot after sm_step, never from the Python worker.
 * Raw exploration uses SNES area ordering and 256 bytes per area (8 areas).
 * Observations and spoiler placement data deliberately stay separate. */
typedef struct {
  uint32_t version,size;
  uint64_t session,revision;
  uint32_t frame,randomized,world_valid;
  uint16_t slot,state,room,area,map_x,map_y;
  uint16_t acquired_items,active_items,acquired_beams,active_beams;
  uint16_t max_health,max_missiles,max_supers,max_power_bombs,max_reserve;
  uint8_t collected[100],item_bits[64],boss_bits[8],events[8],opened_doors[64];
  uint8_t explored_current[256],explored_saved[2048],map_stations[8];
  char seed_fingerprint[65];
} SmTrackerSnapshot;
SM_API int sm_tracker_snapshot(SmTrackerSnapshot *out,uint32_t capacity);
void sm_tracker_new_session(void);
/* All presentation APIs are game-thread only. State bits match PopTracker's
 * MapWidget: 0 cleared, 1 normal, 2 blocked, 4 sequence break, 8 inspect. */
SM_API void sm_tracker_configure(int enabled,int map,int items);
SM_API int sm_tracker_publish(const SmTrackerSnapshot *input,const uint8_t *states,uint32_t count);
/* Extended publication validates native objective evidence as well as inventory;
 * existing v1 callers and their struct size remain compatible. */
SM_API int sm_tracker_publish_objectives(const SmTrackerSnapshot *input,const SmObjectiveSnapshot *objectives,const uint8_t *states,uint32_t count);
SM_API uint32_t sm_tracker_color(unsigned state);
void sm_tracker_render(uint8_t *native_pixels);
const uint8_t *sm_tracker_icon_pixels(int id);

int sm_tracker_reveals_map(void);
/* 0 enabled, 1 reachable remaining (-1 pending), 2 remaining, 3 total. */
SM_API int sm_tracker_area_count(int area,int field);

/* Stable encounter order: Kraid, Phantoon, Draygon, Ridley, Mother Brain,
 * Spore Spawn, Crocomire, Botwoon, Golden Torizo, Bomb Torizo. 255 hides
 * a disabled encounter. Boss markers never change item-check counters. */
#define SM_TRACKER_BOSS_COUNT 10
SM_API int sm_tracker_publish_bosses(const SmTrackerSnapshot *input,const uint8_t *states,uint32_t count);

void sm_tracker_load_assets(void);
