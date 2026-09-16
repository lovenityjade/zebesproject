#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p .tmp
gcc -std=gnu17 -O2 -fno-strict-aliasing -iquote native-core/src Tests/check_scene.c -lm -o .tmp/check-scene
.tmp/check-scene
