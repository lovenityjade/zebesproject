#pragma once
#include "sm_bridge.h"
/* Original menu state/input remain authoritative. Art is presented by Unreal. */
SM_API int sm_gameover_state(int field); /* 0 visible, 1 selection, 2 phase */
SM_API const uint8_t *sm_gameover_labels(void); /* ROM-derived BGRA, 128x48 */
SM_API int sm_test_gameover(void);
