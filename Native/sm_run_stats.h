#pragma once
#include "sm_seed.h"
/* VARIA stat IDs; time fields use 60 Hz native frames in 64-bit counters. */
void sm_stats_open(const char *save_path);
int sm_stats_save(void);
void sm_stats_frame(void);
void sm_stats_add(int id);
void sm_stats_finish(void);
void sm_stats_slot_action(int action,int slot,int other);
SM_API uint64_t sm_stats_value(int id);
SM_API int sm_stats_partial(void);
