#!/usr/bin/env python3
"""Rendered playtest fixtures on gaming-pc only, with private automatic saves."""
import os
from pathlib import Path
import shutil
import subprocess
import sys

root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').exists()
for proc in Path('/proc').iterdir():
    if not proc.name.isdigit():continue
    try:
        if (proc/'comm').read_text().strip()!='plasmashell':continue
        desktop=dict(x.split('=',1) for x in (proc/'environ').read_bytes().decode().split('\0') if '=' in x)
        for key in ('DISPLAY','WAYLAND_DISPLAY','XAUTHORITY','XDG_RUNTIME_DIR','DBUS_SESSION_BUS_ADDRESS'):
            if key in desktop:os.environ[key]=desktop[key]
        break
    except (PermissionError,FileNotFoundError,ProcessLookupError):pass

cases=[('spores',['-SMSporeCheck'],['spore-before','spore-after','spore-motion'],['SM_SPORE_RENDER_PASS']),
       ('presentation',['-SMPlaytestCheck'],['save-toast','speed-booster','map','achievements'],
        ['SM_PLAYTEST_REAL_SAVE_EFFECT_PASS','SM_PLAYTEST_SPEED_EFFECT_PASS']),
       ('recap',['-SMRecapCheck','-SMStartTest=0'],['recap'],
        ['SM_RECAP_CONTROLS_PASS','SM_RECAP_RENDER_PASS'])]
if len(sys.argv)>2:
    cases=[case for case in cases if case[0]==sys.argv[2]]
    assert cases, 'Choose presentation, recap or spores'
for name,flags,captures,markers in cases:
    out=root/'playtest-render-final'/name
    out.mkdir(exist_ok=True,parents=True)
    screenshot_dir=root/'SMUnreal/Saved/SMTests'
    # Clear only known fixture captures, so a stale picture cannot pass a test.
    for label in captures:(screenshot_dir/f'playtest-{label}.png').unlink(missing_ok=True)
    args=[str(root/'SMUnreal/Binaries/Linux/SMUnreal'),'/Engine/Maps/Entry',
          '-windowed','-ResX=1280','-ResY=720','-nosplash','-NoZenService',
          '-ExecCmds=t.MaxFPS 60','-SMWarmup=8500','-SMImageScaling=0','-unattended',*flags]
    with (out/'render.log').open('w') as log:
        subprocess.run(args,cwd=root,stdout=log,stderr=subprocess.STDOUT,check=True,timeout=180)
    text=(out/'render.log').read_text()
    assert all(marker in text for marker in markers),text[-6000:]
    assert 'SM_RECAP_CHECK_FAILED' not in text and 'SM_NATIVE_ERROR' not in text
    for label in captures:
        source=screenshot_dir/f'playtest-{label}.png'
        assert source.exists(),source
        shutil.copy2(source,out/source.name)
    print(f'PLAYTEST_UNREAL_{name.upper()}_PASS',flush=True)
