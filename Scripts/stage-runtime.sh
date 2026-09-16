#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
exec python3 Scripts/stage-runtime.py "$PWD/Build/Package/Linux" --platform Linux
