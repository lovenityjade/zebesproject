#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
engine="${SM_ENGINE:-$HOME/Applications/UnrealEngine-5.8.2}"
mkdir -p .tmp
export TMPDIR="$PWD/.tmp"
"$engine/Engine/Binaries/Linux/UnrealEditor-Cmd" "$PWD/Unreal/SMUnreal.uproject" -run=pythonscript -script="$PWD/Scripts/create_material.py" -unattended -nosplash -NoSound -nullrhi -NoZenService
