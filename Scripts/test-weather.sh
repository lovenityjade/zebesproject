#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
Scripts/play.sh -windowed -ResX=1280 -ResY=720 -SMWeatherTest -SMTestSavedRoom -SMWarmup=8500 -unattended "$@" > .tmp/unreal-weather-test.log 2>&1
python3 Tests/check_weather_capture.py
