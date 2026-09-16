#pragma once
#include "sm_bridge.h"
enum { SM_SEED_FAST_DOORS=1, SM_SEED_NERFED_RAINBOW=2, SM_SEED_HIDDEN_MAP=4, SM_SEED_ORIGINAL_HUD=8, SM_SEED_ORIGINAL_RESERVES=16, SM_SEED_FAST_ELEVATORS=32, SM_SEED_ROUND_ROBIN_CF=64, SM_SEED_RANDO_SPEED=128, SM_SEED_NERFED_CHARGE=256, SM_SEED_BALANCED_SUITS=512, SM_SEED_PROGRESSIVE_SUITS=1024, SM_SEED_RESPIN=2048, SM_SEED_INFINITE_SPACEJUMP=4096, SM_SEED_ITEM_SOUNDS=8192 };
/* Effective per-seed behavior. Vanilla sessions always use original code. */
SM_API void sm_seed_rules_configure(unsigned mask);
SM_API unsigned sm_seed_rules_active(void);
SM_API unsigned sm_seed_rules_capabilities(void);
int sm_seed_rule(unsigned flag);
void sm_seed_align_camera(uint16_t *coordinate);
int sm_seed_crystal_flash(void);
int32_t sm_seed_periodic_damage(int32_t damage);

const uint16_t *sm_seed_pose_entries(unsigned pose);
int sm_seed_pickup_sound(const uint8_t *arguments);
