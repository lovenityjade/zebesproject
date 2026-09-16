#!/usr/bin/env python3
"""Rendered startup/timing/input regression checks in an isolated gaming-pc tree."""
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

assert os.uname().nodename == 'gaming-pc'
root = Path(sys.argv[1]).resolve()
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

editor = os.environ.get('SM_TITLE_EDITOR')
headless = bool(editor and os.environ.get('SM_TITLE_HEADLESS'))
project = root / ('EditorMenu' if editor else 'SMUnreal')
results = root / 'title-results' / ('editor' if editor else 'game')
results.mkdir(parents=True, exist_ok=True)
cases = sys.argv[2:] or ['automatic', 'held-input', 'skip', 'classic']
report = {}
for case in cases:
    number = {'automatic': 0, 'held-input': 1, 'skip': 2, 'classic': 0}[case]
    out = results / case
    out.mkdir(parents=True, exist_ok=True)
    prefix = 'title-classic-' if case == 'classic' else 'title-wide-'
    for old in (project / 'Saved/SMTests').glob(prefix + '*.png'):
        old.unlink()
    args = ([editor, str(project / 'SMUnreal.uproject'), '/Engine/Maps/Entry', '-game'] if editor else
            [str(project / 'Binaries/Linux/SMUnreal'), '/Engine/Maps/Entry'])
    args += ['-windowed', '-ResX=1280', '-ResY=720', '-nosplash', '-NoZenService',
             '-ExecCmds=t.MaxFPS 60', '-unattended', '-SMTitleTest', f'-SMTitleTestCase={number}']
    if case == 'classic':
        args += ['-SMTitleClassic']
    if headless:
        args += ['-nullrhi', '-nosound']
    with (out / 'run.log').open('w') as log:
        # Editor may need its first global shader cache; the in-game watchdog
        # still limits the actual presentation test to 100 seconds.
        subprocess.run(args, cwd=root, stdout=log, stderr=subprocess.STDOUT, timeout=900 if editor else 240, check=True)
    log = (out / 'run.log').read_text()
    assert 'SM_TITLE_TEST PASS' in log and 'SM_NATIVE_READY' in log, log[-5000:]
    assert 'SM_TITLE_TEST FAIL' not in log and 'Assertion failed' not in log, log[-5000:]
    warnings = re.findall(r'SM_STARTUP_WARNING_DONE screen=(\d) seconds=([\d.]+) input=(\d) native_frame=(\d+)', log)
    assert len(warnings) == 2 and all(w[3] == '0' for w in warnings), warnings
    if number == 0:
        assert all(10 <= float(w[1]) < 10.2 for w in warnings), warnings
    elif number == 1:
        assert .9 < float(warnings[0][1]) < 1.2 and 3.9 < float(warnings[1][1]) < 4.2, warnings
    cues = [(int(p), int(f), fn) for p, f, fn in re.findall(r'SM_TITLE_CUE phase=(\d) frame=(\d+) function=([0-9a-f]+)', log)]
    if number != 2:
        # Native no-input baseline captured before changing the renderer.
        assert [(p, f) for p, f, _ in cues] == [(0,1),(1,151),(2,281),(3,597),(4,845),(5,1245),(6,1625),(7,1689)], cues
    captures = list((project / 'Saved/SMTests').glob(prefix + '*.png'))
    if not headless:
        assert len(captures) >= (6 if number == 2 else 11), captures
    for capture in captures:
        shutil.copy2(capture, out / capture.name.removeprefix(prefix))
    if not headless:
        from PIL import Image
        # Runtime font construction can silently produce an empty Canvas draw.
        # Require visible warning text and version pixels, not only cue logs.
        for name in ('warning-photosensitivity', 'warning-transparency'):
            im = Image.open(out / (name + '.png')).convert('RGB')
            w, h = im.size
            heading = im.crop((int(w*.10), int(h*.20), int(w*.90), int(h*.32)))
            body = im.crop((int(w*.10), int(h*.36), int(w*.90), int(h*.78)))
            assert sum(max(p)>100 for p in heading.getdata()) > 300, (name, 'missing heading')
            assert sum(max(p)>100 for p in body.getdata()) > 1000, (name, 'missing body')
            assert max(max(p) for p in im.crop((0, 0, w, int(h*.15))).getdata()) == 0, 'warning background must be black'
        for name in ('warning-photosensitivity', '2026', 'ready', 'save-select'):
            im = Image.open(out / (name + '.png')).convert('RGB')
            w,h=im.size
            corner=im.crop((int(w*.82),int(h*.96),w,h))
            assert sum(max(p)>60 for p in corner.getdata()) > 30, (name, 'missing build label')
    report[case] = {'warnings': warnings, 'native_cues': cues, 'captures': len(captures), 'rendered': not headless, 'pass': True}
    (results / 'verification.json').write_text(json.dumps(report, indent=2)+'\n')
    print(f'TITLE_{case.upper()}_PASS captures={len(captures)}', flush=True)
