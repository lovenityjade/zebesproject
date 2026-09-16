#pragma once
#include "sm_bridge.h"
typedef struct {uint16_t location,plm;} SmDoorIndicator;
#define SM_INDICATOR_MAX 18
int sm_indicators_validate(const SmDoorIndicator *entries,int count,uint64_t patches);
void sm_indicators_select(const SmDoorIndicator *entries,int count);
void sm_indicators_capture(uint8_t *rom);
void sm_indicators_apply(uint8_t *rom,int enabled);
void sm_indicators_room(void);
int sm_indicators_is_plm(unsigned plm);
void sm_indicators_restore(uint8_t *rom);
