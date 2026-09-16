#pragma once
#include "sm_bridge.h"
SM_API void sm_achievement_notify(unsigned newly_unlocked);
void sm_notifications_reset(void);
void sm_notifications_frame(uint8_t *pixels);
