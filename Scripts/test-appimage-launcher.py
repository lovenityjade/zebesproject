#!/usr/bin/env python3
"""Launcher/storage regression with a recording stub, NOT a rendered game test."""
import importlib.util
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile

root = Path(__file__).resolve().parents[1]
assert (root / 'ISOLATED_TEST_DIRECTORY').is_file(), 'Run in an isolated build tree'
spec = importlib.util.spec_from_file_location('appimage_builder', root / 'Scripts/build-appimage.py')
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)

with tempfile.TemporaryDirectory(prefix='appimage-test-', dir=root) as temporary:
    test = Path(temporary)
    appdir = test / 'Read only AppDir é 漢字'
    game = appdir / 'usr/game'
    binary = game / 'SMUnreal/Binaries/Linux/SMUnreal'
    binary.parent.mkdir(parents=True)
    binary.write_text('''#!''' + sys.executable + '''
import json, os, sys
from pathlib import Path
p = Path(os.environ['SM_USER_DATA']) / 'stub-launch-count'
n = int(p.read_text()) if p.exists() else 0
p.write_text(str(n + 1))
print(json.dumps(dict(argv=sys.argv[1:], data=os.environ['SM_USER_DATA'],
    python=os.environ['SM_PYTHON_LIBRARY'], home=os.environ['HOME'],
    cwd=os.getcwd(), unsafePath=os.environ.get('PYTHONPATH'), count=n + 1)))
''')
    # The stub is Python only for recording arguments. It must ignore AppRun's
    # Python environment, which intentionally points at a placeholder runtime.
    binary.write_text(binary.read_text().replace('#!' + sys.executable, '#!' + sys.executable + ' -I'))
    binary.chmod(0o755)
    library = game / 'Runtime/Python/lib/libpython3.11.so.1.0'
    library.parent.mkdir(parents=True)
    library.touch()
    shutil.copy2(root / 'Scripts/AppImage/AppRun', appdir / 'AppRun')
    (appdir / 'AppRun').chmod(0o755)
    before = {str(p.relative_to(appdir)): p.read_bytes() for p in appdir.rglob('*') if p.is_file()}
    for p in appdir.rglob('*'):
        p.chmod(0o555 if p.is_dir() or p == binary or p.name == 'AppRun' else 0o444)
    appdir.chmod(0o555)
    home = test / 'home'
    xdg = test / 'Player data é 漢字'
    env = dict(os.environ, HOME=str(home), XDG_DATA_HOME=str(xdg), PYTHONPATH='/invalid/user/modules')
    def launch(directory=appdir, environment=env):
        result = subprocess.run([str(directory / 'AppRun'), '-windowed', '-TestValue=with spaces'],
                                cwd='/', env=environment, capture_output=True, text=True, check=True)
        return json.loads(result.stdout)
    one, two = launch(), launch()
    assert one['data'] == str(xdg / 'zebesproject') and two['count'] == 2
    assert one['home'] == str(home) and one['unsafePath'] is None
    assert one['argv'][-1] == '-TestValue=with spaces'
    assert '-UserDir=' + str(xdg / 'zebesproject/Unreal') in one['argv']
    assert one['cwd'] == str(game)
    fallback = launch(environment=dict(env, XDG_DATA_HOME='relative-invalid'))
    assert fallback['data'] == str(home / '.local/share/zebesproject')
    assert before == {str(p.relative_to(appdir)): p.read_bytes() for p in appdir.rglob('*') if p.is_file()}
    moved = test / 'Moved AppDir'
    appdir.rename(moved)
    assert launch(moved)['count'] == 3, 'Moving/updating the image must preserve player data'
    # Restore directory permissions for fixture cleanup and failure injection.
    moved.chmod(0o755)
    for p in moved.rglob('*'):
        if p.is_dir():
            p.chmod(0o755)
    (moved / library.relative_to(appdir)).unlink()
    failure = subprocess.run([str(moved / 'AppRun')], env=env, capture_output=True, text=True)
    assert failure.returncode != 0 and 'runtime is missing' in failure.stderr
    assert (xdg / 'zebesproject/stub-launch-count').read_text() == '3'

    # Exercise the staging privacy checks independently of the source gate.
    package = test / 'candidate'
    for filename in ('SMUnreal/Binaries/Linux/SMUnreal', 'Native/build/libsm_native.so',
                     'Randomizer/sm_integration.py', 'Randomizer/upstream/LICENSE', 'SMUnreal/Content/Paks/game.pak'):
        path = package / filename
        path.parent.mkdir(parents=True, exist_ok=True)
        path.touch()
    builder.audit_package(package)
    for filename in ('roms/private.sfc', 'SMUnreal/Saved/profile.json', 'Docs/private.png',
                     'Randomizer/upstream/web/logo.png', 'Randomizer/upstream/patches/artwork.ips'):
        path = package / filename
        path.parent.mkdir(parents=True, exist_ok=True)
        path.touch()
        try:
            builder.audit_package(package)
            raise AssertionError('Private/unreviewed file was accepted: ' + filename)
        except ValueError:
            pass
        path.unlink()
    escaped = package / 'Native/outside'
    escaped.symlink_to(test / 'outside')
    try:
        builder.audit_package(package)
        raise AssertionError('External symlink accepted')
    except ValueError:
        pass
print(json.dumps(dict(launcherStoragePassed=True, stagingRejectionsPassed=True,
                     packagedGameTested=False, appImageProduced=False), indent=2))
