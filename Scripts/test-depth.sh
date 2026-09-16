#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p .tmp
gcc -std=gnu17 -O2 Tests/check_depth.c -o .tmp/check-depth
.tmp/check-depth
Scripts/play.sh -windowed -ResX=1280 -ResY=720 -SMDepthTest -SMWarmup=8500 -SMTestFrames=8550 -unattended "$@" > .tmp/unreal-depth-test.log 2>&1
python3 Tests/check_depth_capture.py
