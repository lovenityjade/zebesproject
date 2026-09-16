#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
engine="${SM_ENGINE:-$HOME/Applications/UnrealEngine-5.8.2}"
export TMPDIR="$PWD/.tmp"
exec "$engine/Engine/Binaries/Linux/UnrealEditor" "$PWD/Unreal/SMUnreal.uproject" /Engine/Maps/Entry -game -nosplash -NoZenService -ExecCmds="t.MaxFPS 60" "$@"
