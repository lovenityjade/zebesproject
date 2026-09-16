#!/usr/bin/env python3
"""Record the isolated gaming-pc runtime artifacts and completed reports."""
import hashlib
import json
import socket
from datetime import datetime, timezone
from pathlib import Path

root = Path(__file__).resolve().parent.parent
assert socket.gethostname() == 'gaming-pc'
assert root.name == 'SuperMetroid-VisualValidation'
reports = {}
paths = [root/'SMUnreal/Saved/SMTests'/f'{name}-verification.json'
         for name in ('native-pause','display')]
paths += [root/'Docs/NativePause/verification.json',
          root/'Docs/NativePause/GamingPC/pixel-verification.json',
          root/'Docs/RoomEditor/verification.json',
          root/'Docs/RoomEditor/browser-verification.json',
          root/'Docs/RoomEditor/runtime-verification.json',
          root/'Docs/DisplayFixes/Unreal/pixel-verification.json']
for path in paths:
    data = json.loads(path.read_text())
    assert data['passed'], (path, data)
    reports[str(path.relative_to(root))] = data
artifacts = {}
for name in ('Native/build/libsm_native.so', 'SMUnreal/Binaries/Linux/SMUnreal',
             'SMUnreal/Content/Paks/SMUnreal-Linux.pak',
             'SMUnreal/Content/Paks/SMUnreal-Linux.ucas',
             'SMUnreal/Content/Paks/SMUnreal-Linux.utoc'):
    with (root/name).open('rb') as file:
        artifacts[name] = hashlib.file_digest(file, 'sha256').hexdigest()
result = dict(passed=True, host=socket.gethostname(),
              recordedAt=datetime.now(timezone.utc).isoformat(),
              path=str(root), artifacts=artifacts, reports=reports,
              scope='Functional and pixel checks in an isolated package; not an all-room or performance guarantee.')
(root/'test-evidence/native-pause-editor-validation.json').write_text(json.dumps(result,indent=2)+'\n')
print('SM_GAMING_PC_VISUAL_PASS', len(reports), 'reports')
