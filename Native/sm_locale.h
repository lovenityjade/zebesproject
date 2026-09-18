#pragma once
#include "sm_seed.h"
/* Language is a presentation preference, independent of every save/seed. */
SM_API void sm_locale_set(int language);
SM_API int sm_locale_get(void);
SM_API const char *sm_locale_text(const char *english);
int sm_locale_next(const char **text);
int sm_locale_length(const char *text);
int sm_locale_base(int codepoint);
int sm_locale_accent(int codepoint);
void sm_locale_message_tiles(uint16_t* body,int index);
void sm_locale_message_choices(uint16_t* row);

int sm_locale_intro_char(unsigned k,unsigned glyph,unsigned position);

void sm_locale_render(uint8_t* pixels);

void sm_locale_message_font(void);
void sm_locale_message_restore(void);
uint16_t sm_locale_message_glyph(int cp);

void sm_locale_menu_font(void);
uint16_t sm_locale_menu_small(int cp);
void sm_locale_menu_big(int cp,uint16_t* top,uint16_t* bottom);
int sm_locale_menu_tilemap(unsigned k,unsigned j);

void sm_locale_intro_setup(void);
int sm_locale_ending_char(unsigned glyph,unsigned position);
void sm_locale_ending_roles(void);

void sm_locale_reset(void);
void sm_locale_caption(const char* text,int x,int y,int visible,int total,int color);

void sm_locale_colony_frame(void);
int sm_locale_area_label(unsigned id,unsigned x,unsigned y,unsigned attributes);
