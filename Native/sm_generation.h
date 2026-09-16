#pragma once
#include "sm_seed.h"
/* Mode 0: vanilla. State 0 pending, 1 working, 2 ready, 3 failed. */
SM_API void sm_generation_configure(int randomized,int ready);
SM_API int sm_generation_state(void);
SM_API int sm_generation_take_request(void);
SM_API void sm_generation_fail(int disk_error);
SM_API int sm_generation_commit(const SmSeedItem *items,int count,const char *fingerprint,const char *sram_path);
int sm_generation_input(void);
/* Front-end mode IDs are stable, independently of the historical seed boolean. */
enum {SM_MODE_VANILLA,SM_MODE_STORY,SM_MODE_BOSS_RUSH,SM_MODE_RANDOMIZER};
SM_API int sm_generation_menu(int field); /* mode, difficulty, row, can-start */
void sm_generation_enter(void);
int sm_generation_cursor(uint16_t object);
void sm_generation_draw(void);
void sm_generation_render(uint8_t *pixels);
int sm_generation_can_start(void);
int sm_generation_is_working(void);
void sm_generation_complete(void);
/* A managed bank binds its shared SRAM to three independently validated plans.
 * Callback actions: 0 mode toggle, 1 settings, 2 copy, 3 clear, 4 persist generated SRAM + seed. */
typedef int (*SmSlotAction)(int action,int slot,int other,void *context);
SM_API void sm_slots_enable(SmSlotAction callback,void *context);
SM_API int sm_slots_set(int slot,int randomized,int ready,const SmSeedItem *items,int count,const char *hash);
SM_API int sm_slots_current(void);
SM_API int sm_slots_copy_sram(uint8_t *out,int capacity);
SM_API int sm_slots_editable(void);
int sm_slots_managed(void);
int sm_slots_select(int slot);
int sm_slots_action(int action,int slot,int other);
void sm_slots_badges(void);
int sm_slots_mode_badges_active(void);
void sm_slots_reset(void);
int sm_slots_commit(const SmSeedItem *items,int count,const char *hash);
