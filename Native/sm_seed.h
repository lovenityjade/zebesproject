#pragma once
#include "sm_bridge.h"
typedef struct { uint32_t address; uint16_t plm; uint16_t kind; } SmSeedItem;
/* Prepare only while core is shut down. Kept separate from menu activation. */
SM_API int sm_seed_stage(const SmSeedItem *items,int count,const char *fingerprint);
SM_API int sm_seed_clear(void);
SM_API int sm_seed_active(void);
SM_API const char *sm_seed_fingerprint(void);
SM_API int sm_seed_rom_item(int index);
SM_API int sm_seed_location_collected(int index);
SM_API int sm_test_seed_room(int awakened);
SM_API int sm_test_seed_pickup_position(void);
SM_API int sm_seed_inventory(int field);
void sm_seed_running(int running);
int sm_seed_apply(uint8_t *rom);
void sm_seed_frame(void);
int sm_seed_morph_collected(void);
void sm_seed_room_items(void);
/* Internal: called only at validated native file/menu boundaries. */
void sm_seed_capture_original(void);
void sm_seed_refresh_maps(void);
int sm_seed_select_plan(const SmSeedItem *items,int count,const char *hash);

int sm_seed_validate_plan(const SmSeedItem *items,int count,const char *hash);

/* Display knowledge only: never writes exploration, station or save flags. */
const uint8_t *sm_seed_map_data(unsigned area);

/* Authored map knowledge: randomized mode or explicit vanilla tracker opt-in. */
int sm_map_fully_known(void);
/* VARIA vanilla_bugfixes: unused vanilla hidden/Chozo Morph PLMs grant
 * Spring Ball (bit 2). Correct their grant in randomized sessions. */
uint16_t sm_seed_equipment_mask(const uint8_t *operand, uint16_t plm);

/* Kind 0 vanilla grant, 1 relic fragment, 2 empty location. */
SM_API int sm_seed_item_kind(int index);
int sm_seed_plm_kind(uint16_t plm);
