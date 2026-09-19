#ifndef SM_SOUNDTRACK_H
#define SM_SOUNDTRACK_H
#include <stdint.h>
#include "sm_bridge.h"
/* Game-thread only. Optional external PCM; no ROM or save data is modified. */
SM_API void sm_soundtrack_configure(int remastered, const char *directory);
SM_API int sm_soundtrack_status(int field); /* 0 preference, 1 track, 2 playing, 3 fallback, 4 preview */
SM_API uint64_t sm_soundtrack_position(void);
SM_API const char *sm_soundtrack_error(void);
SM_API void sm_soundtrack_gameover_configure(const char *filename);
SM_API int sm_soundtrack_gameover_status(void);
/* Boss Rush presentation clock: independent of paused/loading native frames.
 * Files use the same 44.1 kHz stereo MSU1 container. Bits 0/1/2 = valid files. */
SM_API int sm_soundtrack_rush_configure(const char *theme, const char *enter, const char *exit);
/* Bits 3/4 = optional failed/success one-shot recordings. */
SM_API int sm_soundtrack_rush_results_configure(const char *failed,const char *success);
SM_API void sm_soundtrack_rush_render(int16_t *stereo, int frames);
SM_API void sm_soundtrack_rush_transition(int reverse);
SM_API uint64_t sm_soundtrack_rush_status(int field); /* owns bus, cursor, sweep, cue (0 combat, 3 failed, 4 success, 5 death silence), done, total frames */
void sm_soundtrack_reset(void);
uint8_t sm_soundtrack_command(unsigned bank, uint8_t command);
void sm_soundtrack_mix(int16_t *stereo, int frames);
void sm_soundtrack_preview_begin(void);
void sm_soundtrack_preview_end(void);
void sm_soundtrack_preview_mix(int16_t *stereo, int frames);
/* Implemented by the native bridge; queues an SPC command without rerouting. */
void sm_soundtrack_spc_command(uint8_t command);
#endif
