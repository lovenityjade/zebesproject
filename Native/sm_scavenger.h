#pragma once
#include "sm_bridge.h"
/* Separate, append-only contract; existing seed/objective/snapshot ABIs stay
 * unchanged. Entries are exact source words: location ID << 8 | HUD index. */
typedef struct {
  uint32_t version,size;
  uint16_t count,order[17];
} SmScavengerPlan;
SM_API const char *sm_scavenger_catalog_sha256(void);
SM_API int sm_scavenger_configure(int slot,const SmScavengerPlan *plan,const char *catalog);
/* 0 active count; 1 saved progress (-1 if corrupt); 2 next location ID;
 * 3 complete; 4 pause prompt; 5 displayed rank (one-based); 6 started. */
SM_API int sm_scavenger_state(int field);
SM_API int sm_scavenger_entry(int index);
void sm_scavenger_reset(void);
int sm_scavenger_configured(int slot);
void sm_scavenger_activate(int slot);
void sm_scavenger_apply(int randomized);
void sm_scavenger_new_game(void);
void sm_scavenger_capture(uint8_t *rom);
void sm_scavenger_restore(uint8_t *rom);
void sm_scavenger_apply_rom(uint8_t *rom);
int sm_scavenger_allows(unsigned location);
int sm_scavenger_pickup(unsigned location);
int sm_scavenger_ridley_appears(void);
void sm_scavenger_ridley_dead(void);
void sm_scavenger_frame(int hud_enabled);
const char *sm_scavenger_label(void);
