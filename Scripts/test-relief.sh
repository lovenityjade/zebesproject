#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p .tmp
gcc -std=gnu17 -O2 -Wall -Wextra Tests/check_relief.c -o .tmp/check-relief
.tmp/check-relief
Scripts/play.sh -windowed -ResX=1280 -ResY=720 -SMReliefTest -SMWarmup=8500 -SMTestFrames=8550 -unattended "$@" > .tmp/unreal-relief-test.log 2>&1
python3 Tests/check_relief_capture.py
