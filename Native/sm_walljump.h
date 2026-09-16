#pragma once
#include <stdint.h>
void sm_walljump_reset(void);
uint16_t sm_walljump_input(uint16_t buttons);
void sm_walljump_mode(int assisted);
int sm_walljump_assisted(void);
