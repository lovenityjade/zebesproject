/* Windows glue is isolated from SNES headers (which define BYTE/WORD/DWORD). */
#include "sm_platform_io.h"
#undef fopen
#undef rename
#undef remove
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <bcrypt.h>
#include <errno.h>
#include <stdint.h>
#include <stdlib.h>

static wchar_t *wide(const char *utf8) {
  if (!utf8) { errno = EINVAL; return NULL; }
  int size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, NULL, 0);
  if (!size) { errno = EILSEQ; return NULL; }
  wchar_t *result = malloc((size_t)size * sizeof(*result));
  if (!result) { errno = ENOMEM; return NULL; }
  if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, utf8, -1, result, size)) {
    free(result); errno = EILSEQ; return NULL;
  }
  return result;
}
FILE *sm_fopen(const char *path, const char *mode) {
  wchar_t *p = wide(path), *m = wide(mode);
  FILE *file = p && m ? _wfopen(p, m) : NULL;
  free(p); free(m); return file;
}
int sm_rename(const char *from, const char *to) {
  wchar_t *f = wide(from), *t = wide(to);
  int result = -1;
  if (f && t) {
    /* CRT rename refuses an existing destination; save commits need the POSIX
     * replace behavior. Never delete the old save before committing the new one. */
    if (MoveFileExW(f, t, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) result = 0;
    else {
      switch (GetLastError()) {
        case ERROR_FILE_NOT_FOUND: case ERROR_PATH_NOT_FOUND: errno = ENOENT; break;
        case ERROR_ACCESS_DENIED: case ERROR_SHARING_VIOLATION: errno = EACCES; break;
        case ERROR_DISK_FULL: errno = ENOSPC; break;
        case ERROR_NOT_SAME_DEVICE: errno = EXDEV; break;
        default: errno = EIO; break;
      }
    }
  }
  free(f); free(t); return result;
}
int sm_remove(const char *path) {
  wchar_t *p = wide(path);
  int result = p ? _wremove(p) : -1;
  free(p); return result;
}
int sm_windows_random(void *bytes, unsigned long count) {
  return BCryptGenRandom(NULL, bytes, count, BCRYPT_USE_SYSTEM_PREFERRED_RNG) == 0;
}
static INIT_ONCE clock_once = INIT_ONCE_STATIC_INIT;
static uint64_t clock_frequency;
static BOOL CALLBACK init_clock(PINIT_ONCE once, PVOID parameter, PVOID *context) {
  LARGE_INTEGER frequency;
  (void)once; (void)parameter; (void)context;
  QueryPerformanceFrequency(&frequency); clock_frequency = (uint64_t)frequency.QuadPart;
  return TRUE;
}
uint64_t sm_windows_clock_ns(void) {
  LARGE_INTEGER counter;
  InitOnceExecuteOnce(&clock_once, init_clock, NULL, NULL);
  QueryPerformanceCounter(&counter);
  uint64_t ticks = (uint64_t)counter.QuadPart;
  return (ticks / clock_frequency) * 1000000000ull +
         (ticks % clock_frequency) * 1000000000ull / clock_frequency;
}
