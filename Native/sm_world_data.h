#pragma once
#include "sm_bridge.h"
void sm_world_data_load_assets(void);
/* Reviewed immutable patch IDs, not an arbitrary ROM patching ABI.
 * Selection is applied only by the seed transaction; configure does not mutate ROM.
 * Seed generation remains gated until each associated gameplay behavior is tested.
 */
SM_API uint64_t sm_world_data_capabilities(void);
SM_API const char *sm_world_data_catalog_sha256(void);
SM_API int sm_world_data_catalog_supported(const char *sha256);
SM_API int sm_world_data_configure(uint64_t selection);
SM_API uint64_t sm_world_data_selected(void);
SM_API int sm_world_data_configure_order(const uint8_t *ids,int count);
SM_API int sm_world_data_get_order(uint8_t *ids,int capacity);
void sm_world_data_capture(uint8_t *rom);
void sm_world_data_apply(uint8_t *rom,int randomized);
/* Native counterparts of the reviewed VARIA tweak patches. */
int sm_world_chozo_without_spacejump(void);
int sm_world_torizo_wakes(void);
