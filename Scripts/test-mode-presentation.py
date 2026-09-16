#!/usr/bin/env python3
"""Native start-flow and rendered settings checks, isolated on gaming-pc."""
import os
from pathlib import Path
import shutil
import subprocess
import sys

root = Path(sys.argv[1]).resolve()
assert os.uname().nodename == 'gaming-pc'
assert (root / 'ISOLATED_TEST_DIRECTORY').is_file()
for proc in Path('/proc').iterdir():
    if not proc.name.isdigit():
        continue
    try:
        if (proc / 'comm').read_text().strip() != 'plasmashell':
            continue
        desktop = dict(x.split('=', 1) for x in (proc / 'environ').read_bytes().decode().split('\0') if '=' in x)
        for key in ('DISPLAY', 'WAYLAND_DISPLAY', 'XAUTHORITY', 'XDG_RUNTIME_DIR', 'DBUS_SESSION_BUS_ADDRESS'):
            if key in desktop:
                os.environ[key] = desktop[key]
        break
    except (PermissionError, FileNotFoundError, ProcessLookupError):
        pass

out=root/'mode-024-rendered';out.mkdir(exist_ok=True)
for case,name in [(5,'slots'),(6,'vanilla'),(7,'story'),(8,'boss-easy'),(9,'randomizer'),(10,'boss-hardcore')]:
    args=[str(root/'SMUnreal/Binaries/Linux/SMUnreal'),'/Engine/Maps/Entry','-windowed','-ResX=1280','-ResY=720','-nosplash','-NoZenService','-unattended','-ExecCmds=t.MaxFPS 60','-SMCinemaTest',f'-SMCinemaCase={case}','-SMCinemaSample=90']
    with (out/f'{name}.log').open('w') as log:subprocess.run(args,cwd=root,stdout=log,stderr=subprocess.STDOUT,timeout=120,check=True)
    log=(out/f'{name}.log').read_text()
    assert 'SM_CINEMA_UNREAL_PASS' in log and 'FIXTURE_FAILED' not in log and 'MENU_FIXTURE_FAILED' not in log,log[-5000:]
    src=root/'SMUnreal/Saved/SMTests'/f'cinema-{case}-90-effects.png'
    assert src.is_file(),src
    shutil.copy2(src,out/f'{name}.png')
    print(f'MODE_{name.upper()}_RENDERED_PASS',flush=True)
