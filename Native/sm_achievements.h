#pragma once
#include "sm_bridge.h"
#define SM_ACHIEVEMENT_COUNT 45
#define SM_ACHIEVEMENT_MASK ((UINT64_C(1)<<SM_ACHIEVEMENT_COUNT)-1)
typedef struct {int mode,icon,secret;const char *name,*name_fr,*description,*description_fr;} SmAchievementDefinition;
static const SmAchievementDefinition sm_achievement_catalog[SM_ACHIEVEMENT_COUNT]={
#include "sm_achievement_catalog.inc"
};
typedef struct {
 int valid,randomized,items,beams,health,missiles,checks,total,bosses,animals,finished,tablets,required;
 uint64_t frames,session;int slot;
} SmAchievementFacts;
/* Pure rules: missing/disabled seed options never count as accomplished. */
uint64_t sm_achievements_evaluate(const SmAchievementFacts *now,const SmAchievementFacts *previous);
SM_API uint64_t sm_achievement_candidates(void);
void sm_achievements_reset(void);
SM_API void sm_achievement_notify64(uint64_t bits);
SM_API int sm_achievement_set_icon(int icon,const uint8_t *bgra,int width,int height);
