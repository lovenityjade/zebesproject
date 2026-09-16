#!/usr/bin/env python3
"""Add an allowlisted runtime to a freshly cooked package; never stage user data."""
import argparse
from pathlib import Path
import shutil
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[1]
# Runtime services import these packages. Upstream web UI, tools, graphics,
# IPS binaries, recordings and research fixtures are not runtime dependencies.
UPSTREAM_DIRS = {'graph', 'logic', 'patches', 'rando', 'rom', 'solver', 'utils',
                 'standard_presets', 'community_presets', 'rando_presets'}
UPSTREAM_FILES = {'__init__.py', 'randomizer.py', 'solver.py', 'LICENSE', 'README.md'}


def randomizer_files(root):
    for path in sorted((root/'Randomizer').rglob('*')):
        if not path.is_file() or path.is_symlink():
            continue
        rel = path.relative_to(root/'Randomizer')
        if any(part.startswith('.') or part == '__pycache__' for part in rel.parts):
            continue
        if rel.parts[0] == 'upstream':
            local = rel.parts[1:]
            if not local or (len(local) == 1 and local[0] not in UPSTREAM_FILES):
                continue
            if len(local) > 1 and local[0] not in UPSTREAM_DIRS:
                continue
        if path.suffix not in {'.py', '.json', '.md', '.txt'} and path.name != 'LICENSE':
            continue
        yield path, rel


def stage(package, platform):
    subprocess.run([sys.executable, str(ROOT/'Scripts/check-distribution.py')], check=True)
    binary = 'SMUnreal/Binaries/'+('Win64/SMUnreal.exe' if platform == 'Windows' else 'Linux/SMUnreal')
    if not (package/binary).is_file() or not any((package/'SMUnreal/Content/Paks').glob('*.pak')):
        raise ValueError('A freshly cooked package is required')
    # Do not overwrite an install or reuse a previous runtime stage.
    for name in ('Native', 'Randomizer', 'Config', 'Licenses', 'Docs', 'Runtime', 'roms', 'Soundtracks'):
        if (package/name).exists():
            raise ValueError(f'Package already contains {name}; use a fresh output')
    for path in package.rglob('*'):
        rel=path.relative_to(package)
        if path.is_symlink() or (path.is_file() and (path.suffix.lower() in {'.pcm','.mp3','.sfc','.smc','.srm','.sram','.sav','.snapshot'} or 'Saved' in rel.parts)):
            raise ValueError(f'Unreviewed cooked payload: {rel}')
    def copy(source, destination):
        destination=package/destination
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(ROOT/source,destination)
    lib='sm_native.dll' if platform == 'Windows' else 'libsm_native.so'
    copy('Native/build/'+lib,'Native/build/'+lib)
    notices={'native-core/LICENSE.txt':'Native/LICENSE.txt',
             'Unreal/ThirdParty/ImGui/LICENSE.txt':'Licenses/Dear-ImGui.txt',
             'Native/Credits-LICENSE.txt':'Licenses/Native-credits.txt',
             'Native/VariaUI-LICENSE.txt':'Licenses/VARIA-native-UI.txt',
             'Native/WorldPatches-LICENSE.txt':'Licenses/VARIA-native-world.txt',
             'Native/TrackerAssets-LICENSE.txt':'Licenses/Tracker-assets.txt',
             'Randomizer/upstream/LICENSE':'Licenses/VARIA.txt',
             'Unreal/Content/UI/OFL.txt':'Licenses/Montserrat-OFL.txt'}
    for source,destination in notices.items():copy(source,destination)
    for source,rel in randomizer_files(ROOT):copy(source.relative_to(ROOT),Path('Randomizer')/rel)
    # PatchAccess enumerates these even though the native port never executes IPS.
    for name in ('common','vanilla'):
        folder=package/'Randomizer/upstream/patches'/name/'ips'
        folder.mkdir(parents=True,exist_ok=True)
        (folder/'README.txt').write_text('Native port: this directory is intentionally free of IPS payloads.\n')
    # Player/editor decorations are user data, not the development room edits.
    (package/'Config/RoomDecorations').mkdir(parents=True)
    (package/'roms').mkdir()
    for name in ('README.md','LICENSE','ASSET_LICENSE.md','THIRD_PARTY_NOTICES.md','VERSION'):
        copy(name,name)
    copy('Docs/QUALITY-OF-LIFE.md','Docs/QUALITY-OF-LIFE.md')
    if platform == 'Windows':
        source=ROOT/'Runtime/Python'
        if not (source/'python313.dll').is_file() or not (source/'LICENSE.txt').is_file():
            raise ValueError('Pinned embedded CPython 3.13 runtime is required')
        shutil.copytree(source,package/'Runtime/Python',ignore=shutil.ignore_patterns('__pycache__','*.pyc'))
        copy('Runtime/Python/LICENSE.txt','Licenses/CPython.txt')
    print(f'{platform} allowlisted runtime staged at {package}; clean-install test still required')


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('package',type=Path)
    parser.add_argument('--platform',choices=['Linux','Windows'],required=True)
    args=parser.parse_args()
    try:stage(args.package.resolve(strict=True),args.platform)
    except (OSError,ValueError,subprocess.CalledProcessError) as error:
        parser.exit(1,f'Runtime not staged: {error}\n')
