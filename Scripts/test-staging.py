#!/usr/bin/env python3
"""Distribution hygiene using synthetic inputs, never a user's ROM or save."""
from pathlib import Path
import os
import shutil
import subprocess
import tempfile

source = Path(__file__).resolve().parent / 'stage-runtime.sh'
with tempfile.TemporaryDirectory(prefix='zebes-stage-') as temporary:
    root = Path(temporary)
    (root / 'Scripts').mkdir()
    shutil.copy2(source, root / 'Scripts/stage-runtime.sh')
    shutil.copy2(source.with_name('check-distribution.py'), root / 'Scripts/check-distribution.py')
    files = ['Native/build/libsm_native.so', 'native-core/LICENSE.txt',
             'Unreal/ThirdParty/ImGui/LICENSE.txt', 'Unreal/Content/UI/OFL.txt',
             'Randomizer/upstream/LICENSE', 'Randomizer/sm_integration.py',
             'README.md', 'Docs/QUALITY-OF-LIFE.md', 'Docs/private-capture.bin',
             'roms/Super Metroid (Japan, USA) (En,Ja).sfc']
    files += ['Native/' + name + '-LICENSE.txt' for name in
              ['Credits', 'VariaUI', 'WorldPatches', 'TrackerAssets', 'TrackerPalette']]
    for name in files:
        path = root / name
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text('synthetic fixture\n')
    (root / 'Config/RoomDecorations').mkdir(parents=True)
    package = root / 'Build/Package/Linux'
    executable = package / 'SMUnreal/Binaries/Linux/SMUnreal'
    executable.parent.mkdir(parents=True)
    executable.touch()
    executable.chmod(0o755)
    def stage(include_rom=False):
        return subprocess.run(['bash', str(root / 'Scripts/stage-runtime.sh')],
                              env={**os.environ, 'SM_INCLUDE_LOCAL_ROM': str(int(include_rom))},
                              capture_output=True, text=True)
    result = stage()
    assert result.returncode == 0, result.stderr
    assert not list((package / 'roms').iterdir())
    assert sorted(p.name for p in (package / 'Docs').iterdir()) == ['QUALITY-OF-LIFE.md']
    assert (package / 'Randomizer/upstream/LICENSE').is_file()
    rom = package / 'roms/Super Metroid (Japan, USA) (En,Ja).sfc'
    rom.write_text('synthetic fixture\n')
    assert stage().returncode != 0 and rom.exists()
    assert stage(True).returncode != 0, 'Old opt-in must not bypass ROM exclusion'
    rom.unlink()
    (package / 'Docs/old-capture.bin').write_text('keep local evidence')
    assert stage().returncode != 0
    assert (package / 'Docs/old-capture.bin').read_text() == 'keep local evidence'
print('PASS: mandatory ROM exclusion, documentation allowlist, stale-output rejection, preservation')
