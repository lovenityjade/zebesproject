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

editor = os.environ.get('SM_MENU_EDITOR')
project = 'EditorMenu' if editor else 'SMUnreal'
out = root / 'menu-flow-results' / ('editor' if editor else '.')
out.mkdir(exist_ok=True, parents=True)
cases = sys.argv[2:] or ['flow', 'home', 'graphics', 'effects', 'audio', 'input', 'refill',
                         'trackers', 'interface', 'randomizer0', 'randomizer1', 'randomizer5',
                         'randomizer7', 'randomizer8', 'search', 'small']
for case in cases:
    page = 'randomizer0' if case == 'small' else case
    capture = root / project / 'Saved/SMTests' / f'{page}-menu.png'
    capture.unlink(missing_ok=True)
    executable = [editor, str(root / project / 'SMUnreal.uproject'), '/Engine/Maps/Entry', '-game'] if editor else [str(root / 'SMUnreal/Binaries/Linux/SMUnreal'), '/Engine/Maps/Entry']
    args = executable + [
            '-windowed', '-ResX=640' if case == 'small' else '-ResX=1280',
            '-ResY=480' if case == 'small' else '-ResY=720', '-nosplash',
            '-NoZenService', '-ExecCmds=t.MaxFPS 60', '-unattended',
            '-SMMenuFlowTest' if case == 'flow' else f'-SMMenuPreview={page}']
    if editor and case == 'flow':
        args += ['-nullrhi', '-nosound']
    with (out / f'{case}.log').open('w') as log:
        subprocess.run(args, cwd=root, stdout=log, stderr=subprocess.STDOUT, timeout=180, check=True)
    text = (out / f'{case}.log').read_text()
    assert 'SM_NATIVE_READY' in text, text[-5000:]
    assert 'SM_MENU_FLOW_FAILED' not in text and 'Assertion failed' not in text, text[-5000:]
    if case == 'flow':
        assert 'SM_MENU_FLOW_TEST PASS' in text, text[-5000:]
    else:
        if case == 'search':
            assert 'search=Space Jump' in text, 'Search fixture did not display search results'
        assert capture.is_file(), capture
        shutil.copy2(capture, out / f'{case}.png')
    print(f'MENU_{case.upper()}_PASS', flush=True)
