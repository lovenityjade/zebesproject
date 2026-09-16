#pragma once
#include <stdint.h>
/* Host-only presentation; does not alter simulation state. */
extern uint8_t sm_scene_pixels[256*240*4];
extern uint8_t sm_scene_layers[256*240*4];
extern int sm_scene_active, sm_scene_parallax;
extern int sm_scene_weather;
extern int sm_scene_camera_x, sm_scene_camera_y;
void sm_scene_reset(void);

extern uint8_t sm_scene_emission[256*240*4];
extern uint8_t sm_scene_lights[64*60*4];
void sm_scene_finish(void);

extern uint8_t sm_scene_background_mask[256*240];

void sm_hud_stabilize(uint8_t *pixels);
