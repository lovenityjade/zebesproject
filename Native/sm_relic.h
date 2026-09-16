#pragma once
#include "sm_bridge.h"
SM_API void sm_relic_configure(int slot,int required);
SM_API int sm_relic_count(void);
SM_API int sm_relic_required(void);
void sm_relic_reset(void);
void sm_relic_new_session(void);
int sm_relic_collect(uint16_t plm);
uint16_t sm_relic_gfx(uint16_t plm,uint16_t address,int slot);
void sm_relic_reload(int active);
void sm_relic_render(uint8_t *pixels);
int sm_relic_depart(void);
int sm_relic_ending(void);

void sm_relic_icon(uint8_t *out,int width,int x,int y);
/* The original Reserve message lifecycle carries relic text only while armed. */
int sm_relic_message(void);
void sm_relic_message_tiles(uint16_t *body);
void sm_relic_message_closed(void);
void sm_relic_escape_warning(void);
void sm_relic_escape_acknowledge(void);
uint16_t sm_relic_escape_damage(uint16_t damage);
int32_t sm_relic_escape_periodic_damage(int32_t damage);

SM_API int sm_relic_escape_configure(int slot,int minutes);
SM_API int sm_relic_escape_active(void);
void sm_relic_escape_open(const char *save);
void sm_relic_escape_session(void);
void sm_relic_escape_checkpoint(int slot);
void sm_relic_escape_slot_action(int action,int slot,int other);
void sm_relic_escape_frame(void);
void sm_relic_escape_stop(void);
