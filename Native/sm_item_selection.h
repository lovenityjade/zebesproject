#pragma once
#include <stdint.h>
/* Extra host input bits; never forwarded to the SNES controller register. */
void sm_item_selection_reset(void);
uint16_t sm_item_selection_input(uint16_t buttons);
int sm_item_selection_direction(void);
