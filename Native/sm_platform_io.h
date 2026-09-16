#pragma once
/* Native API paths are UTF-8, including paths supplied by Unreal. Include this
 * for every Windows translation unit, including unmodified upstream sources. */
#include <stdio.h>
#ifdef _WIN32
FILE *sm_fopen(const char *path, const char *mode);
int sm_rename(const char *from, const char *to);
int sm_remove(const char *path);
int sm_windows_random(void *bytes, unsigned long count);
#define fopen sm_fopen
#define rename sm_rename
#define remove sm_remove
#endif
