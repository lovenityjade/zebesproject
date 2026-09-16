#!/usr/bin/env bash
# Run from the isolated packaged copy on gaming-pc, never on the workstation.
set -euo pipefail
[[ $(hostname) == gaming-pc ]] || { echo 'Ces tests doivent tourner sur gaming-pc.' >&2; exit 2; }
cd "$(dirname "$0")/.."
[[ $PWD == "$HOME/Games/SuperMetroid-VisualValidation" ]] || exit 2
mkdir -p test-evidence saves
fixture="$PWD/Unreal/Saved/SMTests/HUD-all-items.sram"
export XDG_RUNTIME_DIR="/run/user/$(id -u)"
export SDL_VIDEODRIVER=offscreen
modes=("$@")
[[ ${#modes[@]} -gt 0 ]] || modes=(Pause Electric Combat Display Footstep)
for mode in "${modes[@]}"; do
    case "$mode" in Pause|Electric|Combat|Display|Footstep) ;; *) exit 2 ;; esac
    case "$mode" in Pause) report=native-pause ;; *) report=${mode,,} ;; esac
    rm -f "SMUnreal/Saved/SMTests/$report-verification.json"
    # Sequence capture may skip indices when the renderer advances >1 tick.
    # Remove this test's generated PNGs so an older run cannot fill those gaps.
    rm -f SMUnreal/Saved/SMTests/"$report"-*.png
    [[ $mode != Combat ]] || rm -f SMUnreal/Saved/SMTests/powerbomb-*.png
    extra=()
    fixture="$PWD/Unreal/Saved/SMTests/HUD-all-items.sram"
    if [[ $mode == Combat ]]; then
        extra+=(-SMTestTeleport=0)
        # Original rainy arrival fixture: this sequence selects the sole
        # acquired ammo (Power Bomb), unlike the full-inventory electric test.
        fixture="$PWD/Unreal/Saved/SMPreview/sram.dat"
    elif [[ $mode == Footstep ]]; then
        # Grant equipment after the initial rainy room has loaded. The fully
        # equipped fixture selects the dry Crateria state on entry.
        fixture="$PWD/Unreal/Saved/SMPreview/sram.dat"
    fi
    timeout 180 ./Lancer-Super-Metroid.sh "-SM${mode}Test" \
        -SMTestSavedRoom "-SMTestSave=$fixture" -SMWarmup=8500 \
        -RenderOffscreen -unattended "${extra[@]}" \
        > "test-evidence/${mode}.log" 2>&1
    python3 - "$mode" <<'PY'
import json,sys
from pathlib import Path
name = {'Pause':'native-pause', 'Electric':'electric', 'Combat':'combat', 'Display':'display','Footstep':'footstep'}[sys.argv[1]]
report = Path('SMUnreal/Saved/SMTests') / (name + '-verification.json')
data = json.loads(report.read_text())
assert data['passed'], data
if sys.argv[1] == 'Electric':
    # Read actual persisted profile files in a separate process, not UE's cache.
    import configparser
    profiles = []
    for path in report.parent.glob('validation-*.achievements.ini'):
        ini = configparser.ConfigParser()
        ini.read(path, encoding='utf-8-sig')
        if ini.getint('Local', 'Unlocked', fallback=0) & 24 == 24:
            profiles.append(path.name)
    assert profiles, 'Grapple/Screw achievements not persisted to disk'
    Path('test-evidence/achievements-disk.json').write_text(json.dumps(dict(passed=True, profiles=profiles), indent=2))
print(sys.argv[1], data, flush=True)
PY
done
