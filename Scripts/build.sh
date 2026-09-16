#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p .tmp
export TMPDIR="$PWD/.tmp"
python3 Scripts/bake-lighting.py
python3 Scripts/bake-ceres-background.py
cmake -S Native -B Native/build -DCMAKE_C_COMPILER=/usr/bin/gcc
cmake --build Native/build -j2
engine="${SM_ENGINE:-$HOME/Applications/UnrealEngine-5.8.2}"
"$engine/Engine/Build/BatchFiles/Linux/Build.sh" SMUnrealEditor Linux Development "$PWD/Unreal/SMUnreal.uproject" -MaxParallelActions=2 -NoUBA -NoUBALocal
