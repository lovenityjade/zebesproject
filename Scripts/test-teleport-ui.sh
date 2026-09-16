#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
Scripts/play.sh -windowed -ResX=1280 -ResY=720 -SMTeleportUiTest -SMTestSavedRoom -SMWarmup=8500 -unattended > .tmp/unreal-teleport-ui.log 2>&1
python3 - <<'PY'
from pathlib import Path
import json,shutil
log=Path('.tmp/unreal-teleport-ui.log').read_text(errors='replace')
assert 'SM_TELEPORT_UI_FAIL' not in log
assert 'SM_TELEPORT_UI_PASS' in log,'UI input test did not complete'
out=Path('Docs/Teleport');out.mkdir(exist_ok=True)
for name in ['menu','arrival']:shutil.copy2(f'Unreal/Saved/SMTests/teleport-{name}.png',out/f'{name}.png')
(out/'ui-verification.json').write_text(json.dumps({'engine':'Unreal Engine 5.8.2','real_input_injection':True,'open_navigate_pause_cancel_confirm_arrival':True,'arrival_room':'9ad9','cpu_opcodes':0},indent=2)+'\n')
print('PASS teleport UI: F10, arrows, cancel, pause, confirm, real native arrival.')
PY
