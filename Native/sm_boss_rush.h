#pragma once
#include <stdint.h>

/* Value-only Boss Rush rules/coordinator: no game RAM, save or native RNG writes.
 * The arena adapter must acknowledge a complete arena load and a phase-aware defeat explicitly. */
enum {SM_RUSH_EASY, SM_RUSH_MEDIUM, SM_RUSH_HARD, SM_RUSH_VERY_HARD,
      SM_RUSH_HARDCORE, SM_RUSH_DIFFICULTY_COUNT};
enum {SM_RUSH_BOMB_TORIZO, SM_RUSH_SPORE_SPAWN, SM_RUSH_KRAID,
      SM_RUSH_CROCOMIRE, SM_RUSH_PHANTOON, SM_RUSH_BOTWOON,
      SM_RUSH_DRAYGON, SM_RUSH_GOLDEN_TORIZO, SM_RUSH_RIDLEY,
      SM_RUSH_MOTHER_BRAIN, SM_RUSH_ENCOUNTER_COUNT};
enum {SM_RUSH_IDLE, SM_RUSH_TRANSITION, SM_RUSH_COMBAT,
      SM_RUSH_FAILED, SM_RUSH_FINISHED};
enum {SM_RUSH_HEATED = 1, SM_RUSH_UNDERWATER = 2,
      SM_RUSH_POSITIONAL = 4, SM_RUSH_SCRIPTED_SURVIVAL = 8};
enum {SM_RUSH_HIT, SM_RUSH_SCRIPTED_DRAIN};

typedef struct {
  const char *name;
  uint8_t hp_numerator, hp_denominator;
  uint8_t starting_resources, received_damage;
  uint8_t drop_numerator, drop_denominator, pickup_quantity, one_hit_ko;
} SmBossRushRules;
typedef struct {
  const char *name;
  uint32_t hp[3];
  uint8_t phases, flags;
} SmBossRushEncounter;
typedef struct {
  uint8_t state, difficulty, encounter, mother_brain_sequence;
  uint32_t drop_rng;
  uint64_t frames, encounter_frames, splits[SM_RUSH_ENCOUNTER_COUNT];
} SmBossRush;

const SmBossRushRules *sm_boss_rush_rules(int difficulty);
const SmBossRushEncounter *sm_boss_rush_encounter(int encounter);
int sm_boss_rush_difficulty_step(int difficulty, int direction);
uint32_t sm_boss_rush_hp(int difficulty, int encounter, int phase);
/* Successful shots, rounded UP to five-round capacity. Easy doubles the normal
 * HP budget, not the reduced HP budget. Excludes glass, turrets and misses. */
uint32_t sm_boss_rush_ammo_budget(int difficulty, uint32_t normal_hp,
                                uint32_t damage_per_shot);
uint32_t sm_boss_rush_starting_energy(int difficulty, uint32_t normal_energy);
uint32_t sm_boss_rush_pickup_quantity(int difficulty, uint32_t normal_quantity);
void sm_boss_rush_reset(SmBossRush *run);
int sm_boss_rush_begin(SmBossRush *run, int difficulty, uint32_t drop_seed);
int sm_boss_rush_arena_ready(SmBossRush *run, int encounter);
int sm_boss_rush_complete_encounter(SmBossRush *run, int encounter,
                                   int defeat_confirmed);
void sm_boss_rush_tick(SmBossRush *run, int native_gameplay_tick);
void sm_boss_rush_fail(SmBossRush *run);
int sm_boss_rush_accepts_input(const SmBossRush *run);
int sm_boss_rush_mother_brain_sequence(SmBossRush *run, int active);
/* Damage is POST-suit mitigation. The scripted-drain exemption is valid only
 * during the explicit Mother Brain cinematic interval in Hardcore. */
uint32_t sm_boss_rush_player_damage(const SmBossRush *run, uint32_t damage,
                                   uint32_t health, int source);
/* Native IDs: 1..5 pickups, 6 nothing. Thin successful candidates only. */
int sm_boss_rush_drop(SmBossRush *run, int native_drop);
