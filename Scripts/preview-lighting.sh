#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
# Skip the intro using native inputs; keep the player's SRAM separate.
exec Scripts/play.sh -SMLightingPreview "$@"
