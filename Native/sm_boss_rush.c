#include "sm_boss_rush.h"
#include <limits.h>
#include <string.h>

static const SmBossRushRules rules[SM_RUSH_DIFFICULTY_COUNT] = {
  /* name          HP    resources damage chance quantity OHKO */
  {"EASY",         3, 4, 2,        1,     1, 1,  2,       0},
  {"MEDIUM",       1, 1, 1,        1,     1, 1,  1,       0},
  {"HARD",         2, 1, 1,        1,     1, 1,  1,       0},
  {"VERY HARD",    2, 1, 1,        2,     1, 4,  1,       0},
  {"HARDCORE",     2, 1, 1,        1,     0, 1,  0,       1},
};
/* Ten-arena roster; kits and native combat probes live in sm_rush_runtime. */
static const SmBossRushEncounter encounters[SM_RUSH_ENCOUNTER_COUNT] = {
  {"Bomb Torizo",   {800},             1, 0},
  {"Spore Spawn",   {960},             1, 0},
  {"Kraid",         {1000},            1, 0},
  {"Crocomire",     {0},               1, SM_RUSH_POSITIONAL},
  {"Phantoon",      {2500},            1, 0},
  {"Botwoon",       {3000},            1, 0},
  {"Draygon",       {6000},            1, SM_RUSH_UNDERWATER},
  {"Golden Torizo", {13500},           1, SM_RUSH_HEATED},
  {"Ridley",        {18000},           1, SM_RUSH_HEATED},
  {"Mother Brain",  {3000,18000,36000}, 3, SM_RUSH_SCRIPTED_SURVIVAL},
};

const SmBossRushRules *sm_boss_rush_rules(int difficulty) {
  return difficulty >= 0 && difficulty < SM_RUSH_DIFFICULTY_COUNT
      ? &rules[difficulty] : 0;
}
const SmBossRushEncounter *sm_boss_rush_encounter(int encounter) {
  return encounter >= 0 && encounter < SM_RUSH_ENCOUNTER_COUNT
      ? &encounters[encounter] : 0;
}
int sm_boss_rush_difficulty_step(int difficulty, int direction) {
  if (!sm_boss_rush_rules(difficulty)) return SM_RUSH_EASY;
  int step = direction > 0 ? 1 : direction < 0 ? -1 : 0;
  return (difficulty + step + SM_RUSH_DIFFICULTY_COUNT) % SM_RUSH_DIFFICULTY_COUNT;
}
static uint32_t clamp_u32(uint64_t value) {
  return value > UINT32_MAX ? UINT32_MAX : (uint32_t)value;
}
static uint32_t scaled_hp(const SmBossRushRules *r, uint32_t hp) {
  return clamp_u32(((uint64_t)hp * r->hp_numerator + r->hp_denominator - 1)
                   / r->hp_denominator);
}
uint32_t sm_boss_rush_hp(int difficulty, int encounter, int phase) {
  const SmBossRushRules *r = sm_boss_rush_rules(difficulty);
  const SmBossRushEncounter *e = sm_boss_rush_encounter(encounter);
  return r && e && phase >= 0 && phase < e->phases
      ? scaled_hp(r, e->hp[phase]) : 0;
}
uint32_t sm_boss_rush_ammo_budget(int difficulty, uint32_t hp, uint32_t damage) {
  const SmBossRushRules *r = sm_boss_rush_rules(difficulty);
  if (!r || !damage) return 0;
  uint32_t target = difficulty == SM_RUSH_EASY ? hp : scaled_hp(r, hp);
  uint64_t shots = ((uint64_t)target + damage - 1) / damage;
  /* A four-super budget occupies one five-round pack; Easy grants ten. */
  return clamp_u32(((shots + 4) / 5) * 5 * r->starting_resources);
}
uint32_t sm_boss_rush_starting_energy(int difficulty, uint32_t normal) {
  const SmBossRushRules *r = sm_boss_rush_rules(difficulty);
  return r ? clamp_u32((uint64_t)normal * r->starting_resources) : 0;
}
uint32_t sm_boss_rush_pickup_quantity(int difficulty, uint32_t normal) {
  const SmBossRushRules *r = sm_boss_rush_rules(difficulty);
  return r ? clamp_u32((uint64_t)normal * r->pickup_quantity) : 0;
}
void sm_boss_rush_reset(SmBossRush *run) {
  if (run) memset(run, 0, sizeof(*run));
}
int sm_boss_rush_begin(SmBossRush *run, int difficulty, uint32_t seed) {
  if (!run || run->state != SM_RUSH_IDLE || !sm_boss_rush_rules(difficulty)) return 0;
  sm_boss_rush_reset(run);
  run->state = SM_RUSH_TRANSITION;
  run->difficulty = difficulty;
  run->drop_rng = seed ? seed : 0x5a454245u;
  return 1;
}
int sm_boss_rush_arena_ready(SmBossRush *run, int encounter) {
  if (!run || run->state != SM_RUSH_TRANSITION || encounter != run->encounter
      || !sm_boss_rush_encounter(encounter)) return 0;
  run->state = SM_RUSH_COMBAT;
  return 1;
}
int sm_boss_rush_complete_encounter(SmBossRush *run, int encounter, int complete) {
  if (!run || run->state != SM_RUSH_COMBAT || !complete
      || encounter != run->encounter || !sm_boss_rush_encounter(encounter)) return 0;
  run->splits[encounter] = run->encounter_frames;
  run->encounter_frames = 0;
  run->mother_brain_sequence = 0;
  if (encounter + 1 == SM_RUSH_ENCOUNTER_COUNT) run->state = SM_RUSH_FINISHED;
  else {run->encounter++; run->state = SM_RUSH_TRANSITION;}
  return 1;
}
void sm_boss_rush_tick(SmBossRush *run, int native_tick) {
  if (run && run->state == SM_RUSH_COMBAT && native_tick) {
    if (run->frames < UINT64_MAX) run->frames++;
    if (run->encounter_frames < UINT64_MAX) run->encounter_frames++;
  }
}
void sm_boss_rush_fail(SmBossRush *run) {
  if (run && (run->state == SM_RUSH_COMBAT || run->state == SM_RUSH_TRANSITION)) {
    run->state = SM_RUSH_FAILED;
    run->mother_brain_sequence = 0;
  }
}
int sm_boss_rush_accepts_input(const SmBossRush *run) {
  return run && run->state == SM_RUSH_COMBAT && !run->mother_brain_sequence;
}
int sm_boss_rush_mother_brain_sequence(SmBossRush *run, int active) {
  if (!run || run->state != SM_RUSH_COMBAT || run->encounter != SM_RUSH_MOTHER_BRAIN)
    return 0;
  run->mother_brain_sequence = !!active;
  return 1;
}
uint32_t sm_boss_rush_player_damage(const SmBossRush *run, uint32_t damage,
                                   uint32_t health, int source) {
  const SmBossRushRules *r = run ? sm_boss_rush_rules(run->difficulty) : 0;
  if (!r || run->state != SM_RUSH_COMBAT) return damage;
  if (!damage) return 0;
  if (r->one_hit_ko) {
    if (source == SM_RUSH_SCRIPTED_DRAIN && run->mother_brain_sequence
        && run->encounter == SM_RUSH_MOTHER_BRAIN) return 0;
    return health;
  }
  return clamp_u32((uint64_t)damage * r->received_damage);
}
int sm_boss_rush_drop(SmBossRush *run, int drop) {
  const SmBossRushRules *r = run ? sm_boss_rush_rules(run->difficulty) : 0;
  if (!r || run->state == SM_RUSH_IDLE || drop < 1 || drop > 5) return drop;
  if (!r->drop_numerator) return 6;
  if (r->drop_numerator == r->drop_denominator) return drop;
  /* Independent stream: thinning drops must not consume native boss AI RNG. */
  uint32_t x = run->drop_rng;
  x ^= x << 13; x ^= x >> 17; x ^= x << 5;
  run->drop_rng = x;
  return x % r->drop_denominator < r->drop_numerator ? drop : 6;
}
