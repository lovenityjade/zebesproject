#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
# Run after build.sh and prepare-assets.sh, one operation at a time.
Scripts/play.sh -windowed -ResX=1280 -ResY=720 -SMPixelTest -SMWarmup=8500 -SMTestFrames=8550 -unattended > .tmp/unreal-pixel-test.log 2>&1
python3 Tests/check_pixel_capture.py
