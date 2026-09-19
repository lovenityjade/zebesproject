#include "sm_rush_runtime.h"
#include "sm_achievements.h"
#include "sm_tracker.h"
#include "sm_seed.h"
#include "sm_minimizer.h"
#include "sm_relic.h"
#include "sm_ending.h"
#include "sm_credits.h"
#include "ida_types.h"
#include "variables.h"
static SmAchievementFacts previous;
static int observed_gameplay;
void sm_achievements_reset(void){previous=(SmAchievementFacts){0};observed_gameplay=0;}
uint64_t sm_achievement_candidates(void){
 if(sm_rush_active())return 0;
 if(sm_ending_preview_active())return 0;
 SmTrackerSnapshot s;if(!sm_tracker_snapshot(&s,sizeof(s)))return 0;
 if(s.state==2||s.state==4){sm_achievements_reset();return 0;}
 if(!s.world_valid&&s.state!=39)return 0;
 if(s.world_valid)observed_gameplay=1;
 /* Debug credit launches have no gameplay-to-ending transition. */
 if(s.state==39&&(!observed_gameplay||sm_credits_state(0)))return 0;
 SmAchievementFacts n={.valid=1,.randomized=s.randomized,.items=s.acquired_items,.beams=s.acquired_beams,
  .health=s.max_health,.missiles=s.max_missiles,.slot=s.slot,.session=s.session,
  .finished=s.state==39,.tablets=sm_relic_count(),.required=sm_relic_required(),
  .frames=(((uint64_t)game_time_hours*60+game_time_minutes)*60+game_time_seconds)*60+game_time_frames,
  .animals=!!(s.events[1]&0x80)};
 const int areas[]={1,3,4,2,1,2,4,2},masks[]={1,1,1,1,2,2,2,4};
 for(int i=0;i<8;i++)if(s.boss_bits[areas[i]]&masks[i])n.bosses|=1<<i;
 for(int i=0;i<100;i++)if(!s.randomized||sm_minimizer_check(i)){n.total++;n.checks+=s.collected[i]!=0;}
 uint64_t result=sm_achievements_evaluate(&n,&previous);previous=n;return result;
}
