#!/usr/bin/env python3
"""Linux bridge unload/reload regression using the explicitly bundled Python.

No ROM, player save, or renderer is needed. Run outside a Python test host: the
small C child loads the bridge just as Unreal does between preflight/generation.
"""
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--library', type=Path, required=True)
parser.add_argument('--python', type=Path, required=True, help='Relocatable CPython prefix')
parser.add_argument('--randomizer', type=Path, required=True)
args = parser.parse_args()
source = r'''
#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc, char **argv) {
  if (argc != 3 && argc != 4) return 2;
  if (argc == 4 && !dlopen(argv[3], RTLD_NOW | RTLD_GLOBAL)) return 2;
  const int capacity = 2097152;
  char *out = calloc(capacity, 1);
  if (!out) return 2;
  for (int i = 0; i < 3; ++i) {
    void *h = dlopen(argv[1], RTLD_NOW | RTLD_LOCAL);
    if (!h) { fprintf(stderr, "%s\n", dlerror()); return 2; }
    int (*call)(const char *, const char *, char *, int) = dlsym(h,
        i == 0 ? "sm_randomizer_validate" : "sm_randomizer_generate");
    const char *(*error)(void) = dlsym(h, "sm_randomizer_error");
    if (!call || !error) return 2;
    int n = call(argv[2], "{\"seed\":14092026,\"skill\":\"casual\",\"progression\":\"slow\"}", out, capacity);
    if (argc == 4) {
      if (n != 0 || !strstr(error(), "different Python loaded")) return 1;
      puts("RELOAD_CONFLICT_REJECTED"); return 0;
    }
    if (n <= 0 || n > capacity) { fprintf(stderr, "%s\n", error()); return 1; }
    printf("RELOAD_RESULT %s\n", out); fflush(stdout);
    dlclose(h);
  }
  free(out);
  return 0;
}
'''
prefix = args.python.resolve(strict=True)
library = prefix / 'lib/libpython3.11.so.1.0'
assert library.is_file(), library
env = dict(os.environ, PYTHONHOME=str(prefix), SM_PYTHON_LIBRARY=str(library),
           PYTHONNOUSERSITE='1', LD_LIBRARY_PATH=str(prefix / 'lib'))
env.pop('PYTHONPATH', None)
with tempfile.TemporaryDirectory(prefix='zebes-python-reload-') as folder:
    work = Path(folder)
    (work / 'test.c').write_text(source)
    subprocess.run(['cc', str(work / 'test.c'), '-ldl', '-o', str(work / 'test')], check=True)
    run = subprocess.run([str(work / 'test'), str(args.library.resolve(strict=True)),
                          str(args.randomizer.resolve(strict=True))], env=env,
                         capture_output=True, text=True, timeout=180)
    if run.returncode:
        raise SystemExit(run.stderr or run.stdout)
    results = [json.loads(line.removeprefix('RELOAD_RESULT ')) for line in run.stdout.splitlines()
               if line.startswith('RELOAD_RESULT ')]
    assert len(results) == 3 and all(r['ok'] for r in results), results
    assert results[1]['manifest']['sha256'] == results[2]['manifest']['sha256']
    (work / 'other.c').write_text('int Py_IsInitialized(void) { return 0; }\n')
    subprocess.run(['cc', '-shared', '-fPIC', str(work / 'other.c'), '-o', str(work / 'other.so')], check=True)
    conflict = subprocess.run([str(work / 'test'), str(args.library.resolve()),
                               str(args.randomizer.resolve()), str(work / 'other.so')],
                              env=env, capture_output=True, text=True, timeout=15, check=True)
    assert 'RELOAD_CONFLICT_REJECTED' in conflict.stdout
    missing = subprocess.run([str(work / 'test'), str(args.library.resolve()), str(args.randomizer.resolve())],
                             env=dict(env, SM_PYTHON_LIBRARY=str(work / 'absent.so')),
                             capture_output=True, text=True, timeout=15)
    assert missing.returncode == 1 and 'Bundled Python runtime unavailable' in missing.stderr
    print('SM_PYTHON_RELOAD_PASS: preflight, deterministic regeneration, foreign/missing runtime rejection')
