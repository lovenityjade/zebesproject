# Linux AppImage preparation

Status: **not produced; distribution is blocked**. Do not describe the launcher
or successful compilation as a working, distributable AppImage.

The existing cooked `Build/Package/Linux` is an old development package. It has
research captures, old metadata and extracted artwork. It must not be wrapped
and published. `Scripts/build-appimage.py` runs the source distribution gate
before creating output, then rejects known private/stale staged files. There is
no bypass option. This targeted audit is not a copyright classifier.

## Implemented packaging support

- AppRun starts the cooked Linux executable directly. Unreal's generated shell
  launcher attempts to chmod the binary, which is inappropriate on a read-only
  AppImage mount.
- Persistent data lives at `$XDG_DATA_HOME/zebesproject`, falling back to
  `$HOME/.local/share/zebesproject` when XDG_DATA_HOME is absent or relative.
  The ROM goes in `roms/`; Unreal's `-UserDir` stores settings, logs and save banks
  under `Unreal/Saved/`. Existing development profiles are not imported or moved.
- ROM size, CRC32 and SHA-1 validation still runs before the native core loads on
  every startup. The AppImage does not provide a ROM. `SM_USER_DATA` only changes
  its local storage path on Linux; it does not bypass validation.
- AppRun selects an included CPython 3.11 runtime and disables user Python paths.
  A missing packaged interpreter fails instead of selecting the system Python.
  Folder/development builds keep their existing interpreter selection behavior.
- AppDir uses the project's supplied logo and a desktop entry. The builder keeps
  dependency notices, hashes its payload and tool inputs, and refuses to overwrite
  an existing artifact. No publish/upload operation is performed.

## Build after distribution clearance

Build and cook on gaming-pc in a clean directory, using `Scripts/package.sh`.
The source gate must be resolved first: see [release blockers](README.md#distribution-blockers).
The current staging script also needs a reviewed allowlist for Randomizer's
runtime Python/JSON files instead of copying all upstream media/IPS files.

Provide a tested, relocatable CPython 3.11 prefix containing
`lib/libpython3.11.so.1.0`, its standard library, dependencies and their notices.
The engine's installed runtime is available for isolated integration testing;
dependency/license and minimum-distribution compatibility checks are still
required for the final bundled runtime.

Use checksum-verified local copies of appimagetool and the x86_64 type-2 runtime.
An explicit runtime prevents appimagetool from downloading a moving latest build.

```sh
python3 Scripts/build-appimage.py --check-only --package /path/to/clean/Linux
python3 Scripts/build-appimage.py \
  --package /path/to/clean/Linux \
  --python /path/to/verified/Python \
  --appimagetool /path/to/verified/appimagetool \
  --runtime-file /path/to/verified/runtime-x86_64
```

Expected artifact, **not yet generated**:
`Build/AppImage/The_Zebes_Project-ALPHA-0.24-x86_64.AppImage`, plus SHA-256 and
build inventory. The inventory explicitly records `packagedGameTested: false`;
creation alone does not satisfy runtime acceptance.

## Validation boundary

`Scripts/test-appimage-launcher.py` runs only with `ISOLATED_TEST_DIRECTORY` and
uses a recording stub, not the game. It checks a read-only AppDir, paths with
spaces/non-ASCII characters, persistent storage after moving the AppDir,
argument preservation, missing interpreter rejection, and staging rejection of
ROMs, saves, research captures, upstream media/IPS and escaping symlinks.

Before publishing a real image, run it on a clean Linux user account, first
without a ROM, then with an invalid ROM, then with a privately supplied valid
ROM. Verify import, next-launch CRC rejection after corruption, persistent saves
across image replacement, settings, audio, controller, Vanilla and in-game seed
generation/trackers. Test the FUSE launch and `--appimage-extract` fallback.
Measure library/glibc requirements and validate on the stated minimum distro.

References: [AppDir layout](https://docs.appimage.org/packaging-guide/manual.html),
[AppImage environment](https://docs.appimage.org/packaging-guide/environment-variables.html),
[appimagetool options](https://github.com/AppImage/appimagetool).
