#pragma once
#include "sm_bridge.h"
/* In-process generation. Does not initialize, patch, or reset the running game.
 * Serialized calls. Returns bytes including NUL, or 0 on runtime failure.
 * If capacity is insufficient nothing is copied; reserve 256 KiB normally. */
SM_API int sm_randomizer_generate(const char *module_root, const char *request_json,
                                 char *result, int capacity);
SM_API const char *sm_randomizer_error(void);
/* Read-only compatibility check; seed 0 is a valid unresolved draft. */
SM_API int sm_randomizer_validate(const char *module_root,const char *request_json,char *result,int capacity);

/* Live logic shares the generation mutex and isolated interpreter lifecycle. */
SM_API int sm_tracker_evaluate(const char *module_root,const char *request_json,char *result,int capacity);
