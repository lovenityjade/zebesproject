#!/usr/bin/env python3
"""Wrap a clean cooked Linux package. Never bypass the distribution gate.

Supply a locally verified appimagetool and type-2 runtime; this command never
downloads moving binaries. All builds run in a new directory. Existing releases
and player data are never overwritten. See Docs/Releases/ALPHA-0.24/APPIMAGE.md.
"""
import argparse
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys
import tempfile

ROOT = Path(__file__).resolve().parents[1]


def sha256(path):
    with path.open('rb') as stream:
        return hashlib.file_digest(stream, 'sha256').hexdigest()


def audit_package(package):
    """Reject known private/stale payloads, in addition to source provenance."""
    for required in ('SMUnreal/Binaries/Linux/SMUnreal', 'Native/build/libsm_native.so',
                     'Randomizer/sm_integration.py', 'Randomizer/upstream/LICENSE'):
        if not (package / required).is_file():
            raise ValueError(f'Missing staged runtime file: {required}')
    if not any((package / 'SMUnreal/Content/Paks').glob('*.pak')):
        raise ValueError('A cooked package is required, not an Editor build')
    allowed = {'Engine', 'SMUnreal', 'Native', 'Randomizer', 'Config', 'Licenses',
               'Docs', 'README.md', 'LICENSE', 'ASSET_LICENSE.md',
               'THIRD_PARTY_NOTICES.md', 'NOTICES.txt', 'VERSION', 'roms', 'SMUnreal.sh',
               'Lancer-Super-Metroid.sh', 'Manifest_NonUFSFiles_Linux.txt',
               'Manifest_UFSFiles_Linux.txt', 'Manifest_DebugFiles_Linux.txt'}
    for entry in package.iterdir():
        if entry.name not in allowed:
            raise ValueError(f'Unreviewed package entry: {entry.name}')
    for path in package.rglob('*'):
        rel = path.relative_to(package)
        if path.is_symlink() and not path.resolve().is_relative_to(package):
            raise ValueError(f'Link escapes package: {rel}')
        if not path.is_file():
            continue
        if (set(rel.parts) & {'.git', 'Saved', '__pycache__', 'Soundtracks', 'roms'} or
                path.suffix.lower() in {'.sfc', '.smc', '.srm', '.sav', '.sram', '.snapshot', '.log'} or
                path.name in {'.env', 'ISOLATED_TEST_DIRECTORY'}):
            raise ValueError(f'Private or stale file in package: {rel}')
        if rel.parts[0] == 'Docs' and rel != Path('Docs/QUALITY-OF-LIFE.md'):
            raise ValueError(f'Research/capture data must not ship: {rel}')
        if rel.parts[0] == 'Randomizer' and path.suffix.lower() not in {'.py', '.json', '.md', '.txt'} and path.name != 'LICENSE':
            raise ValueError(f'Unreviewed randomizer asset: {rel}')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--package', type=Path, default=ROOT / 'Build/Package/Linux')
    parser.add_argument('--python', type=Path, help='Relocatable CPython 3.11 prefix with lib/python3.11/LICENSE.txt')
    parser.add_argument('--appimagetool', type=Path, help='Verified local appimagetool executable')
    parser.add_argument('--runtime-file', type=Path, help='Verified local x86_64 type-2 runtime')
    parser.add_argument('--engine-notices', type=Path, required=True, help='Engine/Source/ThirdParty/Licenses from the packaging engine')
    parser.add_argument('--output', type=Path, default=ROOT / 'Build/AppImage')
    parser.add_argument('--check-only', action='store_true')
    args = parser.parse_args()
    # This must happen before creating/copying any output or running build tools.
    subprocess.run([sys.executable, str(ROOT / 'Scripts/check-distribution.py')], check=True)
    package = args.package.resolve(strict=True)
    audit_package(package)
    if args.check_only:
        print('Known source/staging checks passed; full provenance and runtime review still required.')
        return
    if not all((args.python, args.appimagetool, args.runtime_file)):
        parser.error('--python, --appimagetool and --runtime-file are required to build')
    python = args.python.resolve(strict=True)
    tool = args.appimagetool.resolve(strict=True)
    runtime = args.runtime_file.resolve(strict=True)
    for path in (python / 'lib/libpython3.11.so.1.0', python / 'lib/python3.11/encodings/__init__.py',
                 python / 'lib/python3.11/LICENSE.txt', tool, runtime):
        if not path.is_file():
            raise ValueError(f'Missing build input: {path}')
    version = (ROOT / 'VERSION').read_text().strip()
    if not re.fullmatch(r'ALPHA-\d+\.\d+', version):
        raise ValueError('Invalid VERSION')
    output = args.output.resolve()
    if output.is_relative_to(package):
        raise ValueError('Output cannot be inside the input package')
    output.mkdir(parents=True, exist_ok=True)
    name = f'The_Zebes_Project-{version}-x86_64.AppImage'
    final = output / name
    for candidate in (final, output / (name + '.sha256'), output / (name + '.build.json')):
        if candidate.exists():
            raise ValueError(f'Refusing to overwrite: {candidate}')
    with tempfile.TemporaryDirectory(prefix='.appimage-', dir=output) as temporary:
        work = Path(temporary)
        appdir = work / 'TheZebesProject.AppDir'
        game = appdir / 'usr/game'
        shutil.copytree(package, game, symlinks=True,
                        ignore=shutil.ignore_patterns('*.debug', '*.sym', '*.pdb', '*.target'))
        # Ordinary package launchers are not used on a read-only mount.
        for launcher in ('SMUnreal.sh', 'Lancer-Super-Metroid.sh'):
            (game / launcher).unlink(missing_ok=True)
        shutil.copytree(python, game / 'Runtime/Python', symlinks=True,
                        ignore=shutil.ignore_patterns('__pycache__', 'site-packages', 'test', '*.pyc', '*.a', 'pkgconfig'))
        for path in (game / 'Runtime/Python').rglob('*'):
            if path.is_symlink() and not path.resolve().is_relative_to(game / 'Runtime/Python'):
                raise ValueError(f'Python runtime is not relocatable: {path.relative_to(appdir)}')
        for filename in ('LICENSE', 'ASSET_LICENSE.md', 'THIRD_PARTY_NOTICES.md', 'VERSION'):
            shutil.copy2(ROOT / filename, game / filename)
        (game / 'Licenses').mkdir(exist_ok=True)
        if not args.engine_notices.is_dir():
            raise ValueError('Engine third-party notices are required')
        shutil.copytree(args.engine_notices,game/'Licenses/Unreal-third-party')
        shutil.copy2(python / 'lib/python3.11/LICENSE.txt', game / 'Licenses/CPython.txt')
        for notice in ('AppImage-runtime.txt','AppImage-source.md','libffi.txt'):
            shutil.copy2(ROOT/'Licenses'/notice,game/'Licenses'/notice)
        shutil.copy2(ROOT / 'Scripts/AppImage/AppRun', appdir / 'AppRun')
        shutil.copy2(ROOT / 'Scripts/AppImage/zebesproject.desktop', appdir / 'zebesproject.desktop')
        shutil.copy2(ROOT / 'zebesproject-logo.png', appdir / 'zebesproject.png')
        (appdir / '.DirIcon').symlink_to('zebesproject.png')
        for path in (appdir / 'AppRun', game / 'SMUnreal/Binaries/Linux/SMUnreal'):
            path.chmod(0o755)
        subprocess.run(['desktop-file-validate', str(appdir / 'zebesproject.desktop')], check=True)
        manifest = {str(p.relative_to(appdir)): sha256(p) for p in sorted(appdir.rglob('*')) if p.is_file()}
        env = dict(os.environ, ARCH='x86_64', VERSION=version, APPIMAGE_EXTRACT_AND_RUN='1')
        # Explicit runtime avoids appimagetool downloading a moving latest build.
        built = work / name
        subprocess.run([str(tool), '--runtime-file', str(runtime), '--no-appstream',
                        '--mksquashfs-opt', '-processors', '--mksquashfs-opt', '2',
                        str(appdir), str(built)], env=env, check=True)
        with built.open('rb') as stream:
            header = stream.read(11)
        if header[:4] != b'\x7fELF' or header[8:11] != b'AI\x02':
            raise ValueError('Tool did not produce a type-2 AppImage')
        built.chmod(0o755)
        digest = sha256(built)
        # Link publishes atomically and refuses an existing destination.
        os.link(built, final)
        with (output / (name + '.sha256')).open('x') as stream:
            stream.write(f'{digest}  {name}\n')
        with (output / (name + '.build.json')).open('x') as stream:
            json.dump(dict(version=version, sha256=digest, files=manifest,
                           appimagetoolSha256=sha256(tool), runtimeSha256=sha256(runtime),
                           packagedGameTested=False), stream, indent=2)
            stream.write('\n')
    print(final)
    print('Created; a clean-install runtime test is still required before publication.')


if __name__ == '__main__':
    try:
        main()
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        print(f'AppImage not created: {error}', file=sys.stderr)
        sys.exit(1)
