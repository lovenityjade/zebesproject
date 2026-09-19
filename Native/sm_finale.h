#ifndef SM_FINALE_H
#define SM_FINALE_H
#include <stdint.h>
#include "sm_bridge.h"
SM_API void sm_set_finale_textures(int ready);
int sm_finale_hyper_enabled(void);
void sm_finale_reset(void);
void sm_finale_frame(void);
void sm_finale_phase(int phase);
void sm_finale_baby(unsigned index);
void sm_finale_hit(void);
void sm_finale_shot(unsigned index);
int sm_finale_state(int field);
int sm_finale_enabled(void);
int sm_finale_music(void);
int sm_finale_gain(void);
#endif
