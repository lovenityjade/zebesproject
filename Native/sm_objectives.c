#include "sm_objectives.h"
#include "sm_objective_events.h"
#include "sm_map_exploration.h"
#include "sm_areas.h"
#include "sm_seed.h"
#include "sm_scavenger.h"
#include "sm_minimizer.h"
#include "sm_escape.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
#include "sm_objectives.inc"
_Static_assert(sizeof(SmObjectivePlan)==272,"Objective ABI changed");
_Static_assert(sizeof(SmObjectiveSnapshot)==472,"Objective snapshot ABI changed");
static SmObjectivePlan configurations[4],applied;
static int selected=-1,cursor,sfx_pending;
static int active(void){return sm_seed_active() && applied.count;}
static int bits(unsigned n){int count=0;while(n){count+=n&1;n>>=1;}return count;}
const char *sm_objectives_catalog_sha256(void){return objective_catalog;}
void sm_objectives_reset(void){memset(configurations,0,sizeof(configurations));memset(&applied,0,sizeof(applied));selected=-1;cursor=sfx_pending=0;}
int sm_objectives_configure(int slot,const SmObjectivePlan *plan,const char *catalog){
  if(slot<0 || slot>=4 || !catalog || strcmp(catalog,objective_catalog))return 0;
  if(!plan){memset(&configurations[slot],0,sizeof(*plan));return 1;}
  if((plan->version<1 || plan->version>5) || plan->size!=sizeof(*plan) || !plan->count || plan->count>18 || !plan->required || plan->required>plan->count || plan->flags&~(plan->version==5?31:plan->version>=3?15:7) || plan->item_mask&~0xf32f || plan->beam_mask&~0x100f)return 0;
  uint64_t seen=0;
  for(int i=0;i<18;i++){
    unsigned id=plan->goals[i];
    if(i>=plan->count){if(id)return 0;continue;}
    if(id>=sizeof(objective_conditions)/sizeof(objective_conditions[0]) || (!objective_conditions[id].supported && !(id==16 && plan->version>=2 && sm_scavenger_configured(slot))) || (seen&(UINT64_C(1)<<id)))return 0;
    if((plan->flags&SM_OBJECTIVE_BT_SLEEP) && objective_conditions[id].kind==OBJ_KIND_ROBOTS)return 0;
    seen|=UINT64_C(1)<<id;
  }
  if(plan->version<3 && (plan->version==2)!=!!(seen&(UINT64_C(1)<<16)))return 0;
  if(plan->version<4 && (plan->version==3)!=!!(plan->flags&SM_OBJECTIVE_FAST_TOURIAN))return 0;
  int mini=!!sm_minimizer_configured(slot),escape=plan->version==5;
  if(!escape && (plan->version==4)!=mini)return 0;
  if(escape && (!sm_escape_configured(slot) || !!(plan->flags&SM_OBJECTIVE_DISABLED_TOURIAN)!=!!(sm_escape_slot_flags(slot)&SM_ESCAPE_DISABLED_TOURIAN) || (plan->flags&24)==24))return 0;
  for(int i=0;i<100;i++)if(plan->item_counted[i]>1 || plan->area_counted[i]>1)return 0;
  static const uint16_t full_enemies[6]={62,19,22,20,5,17};
  for(int f=0;f<6;f++)if(plan->enemy_totals[f]!=(mini?sm_minimizer_enemy_total(slot,f):full_enemies[f]))return 0;
  for(int r=0;r<12;r++)if(plan->map_totals[r]!=(mini?sm_minimizer_map_total(slot,r):
    sm_map_exploration_total(sm_areas_configured(slot),r)-(!sm_areas_configured(slot) && r==1 && (plan->flags&SM_OBJECTIVE_FAST_TOURIAN)?1:0))-(escape?sm_escape_map_offset(slot,r):0))return 0;
  if(mini)for(int i=0;i<100;i++)if((plan->item_counted[i] || plan->area_counted[i]) && !sm_minimizer_slot_check(slot,i))return 0;
  configurations[slot]=*plan;return 1;
}
int sm_objectives_validate_slot(int slot){
  if(slot<0 || slot>=4)return 0;
  const SmObjectivePlan *p=&configurations[slot];if(!p->count)return 1;
  for(int i=0;i<p->count;i++)if(p->goals[i]==16 && !sm_scavenger_configured(slot))return 0;
  /* Revalidate pending dependencies when committing, including membership
   * edits after objective configuration. The previous applied world is intact. */
  return sm_objectives_configure(slot,p,objective_catalog);
}
void sm_objectives_activate(int slot){selected=slot>=0 && slot<4?slot:-1;}
void sm_objectives_apply(int randomized){applied=randomized && selected>=0?configurations[selected]:(SmObjectivePlan){0};cursor=sfx_pending=0;}
static void item_counts(int region,int *have,int *total){
  *have=*total=0;
  for(int i=0;i<100;i++)if((region<0?applied.item_counted[i]:applied.area_counted[i] && objective_location_regions[i]==region)){
    ++*total;*have+=sm_seed_location_collected(i)!=0;
  }
}
static int condition(int index,int *have,int *total,int *percent){
  unsigned id=applied.goals[index],arg=objective_conditions[id].arg;
  int kind=objective_conditions[id].kind,n=0;
  *have=0;*total=1;*percent=-1;
  switch(kind){
    case OBJ_KIND_EVENT:
      if(id==16){*have=sm_scavenger_state(1);if(*have<0)*have=0;*total=sm_scavenger_state(0);return sm_scavenger_state(3);}
      *have=sm_objective_event(arg);return *have;
    case OBJ_KIND_BOSSES:case OBJ_KIND_MINIBOSSES:
      for(int i=0;i<4;i++)*have+=sm_objective_event(kind==OBJ_KIND_BOSSES?objective_boss_events[i]:objective_miniboss_events[i]);
      *total=arg;return *have>=*total;
    case OBJ_KIND_NOTHING:*have=sm_escape_nothing_allowed();return *have;
    case OBJ_KIND_ITEM_PERCENT:
      item_counts(-1,have,&n);*total=(n*arg+99)/100;*percent=n?*have*100/n:0;return *have>=*total;
    case OBJ_KIND_UPGRADES:
      *have=bits(collected_items&applied.item_mask)+bits(collected_beams&applied.beam_mask);
      *total=bits(applied.item_mask)+bits(applied.beam_mask);
      return collected_items==applied.item_mask && collected_beams==applied.beam_mask;
    case OBJ_KIND_AREA_CLEAR:
      item_counts(arg,have,total);return sm_objective_event(132+arg);
    case OBJ_KIND_MAP_PERCENT:
      *have=sm_map_exploration_value(-1,0);for(int r=0;r<12;r++)n+=applied.map_totals[r];
      *total=(n*arg+99)/100;
      /* VARIA's original percentage presentation uses 8-tile units. */
      *percent=n>=8?((*have>>3)*100)/(n>>3):0;if(*percent>99)*percent=99;
      return *have>=*total;
    case OBJ_KIND_AREA_EXPLORE:
      *have=sm_map_exploration_value(arg,0);*total=applied.map_totals[arg];*percent=*total?*have*100/ *total:0;return *have>=*total;
    case OBJ_KIND_ROBOTS:
      for(int i=0;i<4;i++)*have+=sm_objective_event(objective_robot_events[i]);*total=4;return *have==4;
    case OBJ_KIND_ANIMALS:
      *have=sm_objective_event(158)+sm_objective_event(159);*total=2;return area_index==1 && *have==2;
    case OBJ_KIND_ENEMY_FAMILY:
      *have=sm_objective_enemy_count(arg,0);*total=applied.enemy_totals[arg];return sm_objective_event(objective_family_events[arg]);
  }
  return 0;
}
int sm_objectives_value(int index,int field){
  if(!active() || index<0 || index>=applied.count)return -1;
  int have,total,percent,met=condition(index,&have,&total,&percent);
  switch(field){case 0:return sm_objective_event(162+2*index);case 1:return have;case 2:return total;case 3:return percent;case 4:return met;default:return -1;}
}
int sm_objectives_state(int field){
  if(!active())return 0;
  int completed=0;for(int i=0;i<applied.count;i++)completed+=sm_objective_event(162+2*i);
  switch(field){case 0:return applied.count;case 1:return applied.required;case 2:return completed;case 3:return completed>=applied.required?0:applied.required-completed;case 4:return sm_objective_event(10);case 5:return sm_objective_event(160);case 6:return applied.flags;case 7:return !(applied.flags&SM_OBJECTIVE_HIDDEN) || sm_objective_event(161);default:return 0;}
}
int sm_objectives_snapshot(SmObjectiveSnapshot *out,uint32_t capacity){
  if(!out || capacity<sizeof(*out))return 0;
  memset(out,0,sizeof(*out));out->version=1;out->size=sizeof(*out);
  memset(out->goals,0xff,sizeof(out->goals));memset(out->values,0xff,sizeof(out->values));
  if(!active())return 1;
  for(int field=0;field<8;field++)out->state[field]=sm_objectives_state(field);
  for(int i=0;i<applied.count;i++){
    int have,total,percent,met=condition(i,&have,&total,&percent);
    out->goals[i]=applied.goals[i];
    out->values[i][0]=sm_objective_event(162+2*i);out->values[i][1]=have;
    out->values[i][2]=total;out->values[i][3]=percent;out->values[i][4]=met;
  }
  return 1;
}
void sm_objectives_frame(void){
  if(!active() || game_state!=8)return;
  if(room_ptr==0xa66a)sm_objective_event_mark(161);
  int region=sm_map_exploration_region(),have,total,percent;
  if(region>0 && region<11){
    item_counts(region,&have,&total);
    if(have)sm_objective_event_mark(147+region);
    if(have==total)sm_objective_event_mark(132+region);
  }
  if(sm_objective_event(160))return;
  if(!sm_objective_event(162+cursor*2) && condition(cursor,&have,&total,&percent)){
    sm_objective_event_mark(162+cursor*2);sfx_pending=1;
  }
  if(++cursor>=applied.count){
    cursor=0;int complete=sm_objectives_state(2);
    if(complete==applied.count)sm_objective_event_mark(160);
    if(!sm_objective_event(10)){
      if(complete>=applied.required){SetEventHappened(10);if(sm_escape_trigger()){sfx_pending=0;return;}}
      if(sfx_pending && (applied.flags&SM_OBJECTIVE_SFX))QueueSfx2_Max15(0x19);
    }
    sfx_pending=0;
  }
}
int sm_objectives_check_index(void){return active()?cursor:-1;}
int sm_objectives_goal(int index){return active() && index>=0 && index<applied.count?applied.goals[index]:-1;}
int sm_objectives_statue_event(unsigned operand){
  if(!active())return 1;
  if(operand==0x8402 || operand==0x846a || operand==0x84d2 || operand==0x853a)return sm_objective_event(10);
  return 1;
}
