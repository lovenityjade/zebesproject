#pragma once
#include "sm_bridge.h"
SM_API void sm_set_refill_before_save(int enabled);
SM_API int sm_refill_before_save(void);
/* Call only after accepting an actual station/ship save, before SRAM capture. */
void sm_refill_at_save(void);
