#pragma once
#include <stdint.h>
void sm_sprite_view_reset(void);
void sm_sprite_view_record(int byte_index, uint16_t base_x, uint16_t tile_offset, uint16_t base_y, uint8_t offset_y);
int sm_sprite_view_x(int word_index, uint32_t oam_words, int *x);
void sm_sprite_view_set_gui(int gui);
int sm_sprite_view_gui(int word_index,uint32_t oam_words);

int sm_sprite_view_y(int word_index,uint32_t oam_words,int *y);
