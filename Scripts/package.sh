#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
python3 Scripts/check-distribution.py
engine="${SM_ENGINE:-$HOME/Applications/UnrealEngine-5.8.2}"
mkdir -p .tmp
export TMPDIR="$PWD/.tmp"
python3 Scripts/bake-lighting.py
python3 Scripts/bake-ceres-background.py
cmake -S Native -B Native/build
cmake --build Native/build -j2
material=Unreal/Content/SM/M_Present.uasset
if [[ ! -f "$material" || Shaders/Present.usf -nt "$material" || Shaders/Cinematics.usf -nt "$material" || Shaders/Environment.usf -nt "$material" || Scripts/create_material.py -nt "$material" ]]; then
  Scripts/prepare-assets.sh
fi
"$engine/Engine/Build/BatchFiles/RunUAT.sh" BuildCookRun \
  -project="$PWD/Unreal/SMUnreal.uproject" -noP4 -platform=Linux \
  -clientconfig=Development -build -cook -map=/Engine/Maps/Entry \
  -stage -pak -archive -archivedirectory="$PWD/Build/Package" \
  -unattended -utf8output -ubtargs="-MaxParallelActions=2 -NoUBA"
Scripts/stage-runtime.sh
