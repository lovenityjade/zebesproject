#pragma once
#include "sm_bridge.h"
void sm_runs_open(const char *save);
int sm_runs_save(void);
void sm_runs_frame(void);
void sm_run_new_game(void);
void sm_run_finish(void);
void sm_run_slot_action(int action,int slot,int other);
/* Category: 0 casual, 1 speedrun No QoL, 2 speedrun QoL. New empty slots only. */
SM_API int sm_run_configure(int slot,int category);
SM_API int sm_run_has_completion(const char *save,int slot);
SM_API int sm_run_import_ngplus(const char *source,int source_slot,int target_slot);
SM_API int sm_run_state(int field); /* 0 category, 1 NG+, 2 eligible, 3 finished */
SM_API uint64_t sm_run_time(int real_time); /* native frames / real milliseconds */
SM_API void sm_run_invalidate(int reason);
int sm_run_assists_allowed(void);
uint32_t sm_run_enemy_damage(uint32_t amount);
void sm_run_enemy_spawn(int index);
uint16_t sm_run_contact_damage(uint16_t amount);
void sm_run_render(uint8_t *pixels);
