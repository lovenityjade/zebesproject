#pragma once
#include "sm_bridge.h"
/* Presentation handshake. Native simulation advances only while loading or
 * fighting; Unreal owns the real-time wireframe transition. */
enum {SM_RUSH_OFF,SM_RUSH_OUT,SM_RUSH_LOADING,SM_RUSH_READY,SM_RUSH_FIGHT,
      SM_RUSH_DEATH,SM_RUSH_END,SM_RUSH_GAMEOVER,SM_RUSH_RESULTS};
SM_API int sm_rush_info(int field); /* stage, encounter, difficulty, practice,
  * ever cheated, first entry, native game frames, scripted survival, deaths, continues */
SM_API int sm_rush_command(int command); /* 0 load, 1 resume, 2 show game over,
  * 3 show results, 4 restore menu, 5 debug skip, 6 continue,
  * 7 restart run, 8 end at title */
SM_API int sm_rush_practice(int enabled); /* -1 query; persists for this process */
SM_API int sm_rush_request(int difficulty);
SM_API const char *sm_rush_name(void);
SM_API uint64_t sm_rush_split(int encounter);
int sm_rush_active(void);
void sm_rush_close(void);
int sm_rush_before(uint16_t *buttons);
void sm_rush_after(void);
void sm_rush_load_room(void);
void sm_rush_draw_pose(void);
uint16_t sm_rush_damage(uint16_t damage);
uint32_t sm_rush_periodic(uint32_t damage);
uint32_t sm_rush_enemy_damage(uint32_t damage);
uint32_t sm_rush_crocomire_recoil(uint32_t distance);
int sm_rush_drop(int drop);
int sm_rush_pickup(int amount);
void sm_rush_mb_sequence(int enabled);
int sm_rush_mb_visual(void);
int sm_rush_mb_finish(void);
int sm_rush_block_doors(void);

void sm_rush_render(uint8_t *pixels);

SM_API const uint8_t *sm_rush_screen(int selection,int paused);
