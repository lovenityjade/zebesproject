#pragma once
#include "sm_bridge.h"
enum {SM_OBJECTIVE_SFX=1,SM_OBJECTIVE_HIDDEN=2,SM_OBJECTIVE_BT_SLEEP=4,SM_OBJECTIVE_FAST_TOURIAN=8,SM_OBJECTIVE_DISABLED_TOURIAN=16};
/* v1: full original/area layouts, ordinary Tourian. Unsupported modes remain
 * rejected until their execution and effective masks have a native contract. */
typedef struct {
  uint32_t version,size;
  uint16_t count,required,flags,item_mask,beam_mask;
  uint8_t goals[18],item_counted[100],area_counted[100];
  uint16_t enemy_totals[6],map_totals[12];
} SmObjectivePlan;
SM_API const char *sm_objectives_catalog_sha256(void);
SM_API int sm_objectives_configure(int slot,const SmObjectivePlan *plan,const char *catalog);
/* state: 0 count, 1 required, 2 completed, 3 remaining, 4 required met,
 * 5 all met, 6 flags, 7 revealed. */
SM_API int sm_objectives_state(int field);
/* value: 0 latched completion, 1 amount, 2 target, 3 display percentage
 * (-1 when not a percentage), 4 current condition. Invalid/inactive=-1. */
SM_API int sm_objectives_value(int index,int field);
/* Read-only companion to the unchanged tracker v1 snapshot. State/value fields
 * have the meanings above. Inactive IDs and values are -1. Game-thread only. */
typedef struct {
  uint32_t version,size;
  int32_t state[8],goals[18],values[18][5];
} SmObjectiveSnapshot;
SM_API int sm_objectives_snapshot(SmObjectiveSnapshot *out,uint32_t capacity);
void sm_objectives_reset(void);
int sm_objectives_validate_slot(int slot);
void sm_objectives_activate(int slot);
void sm_objectives_apply(int randomized);
void sm_objectives_frame(void);
/* The HUD follows the original evaluator's current index, including its
 * reset to zero at the end of each list. This is transient, not save data. */
int sm_objectives_check_index(void);
int sm_objectives_goal(int index);
int sm_objectives_statue_event(unsigned operand);
