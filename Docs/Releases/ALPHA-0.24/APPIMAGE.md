> **Public source layout:** development scripts and test harnesses referenced below
> are retained in the private development repository. For current public build
> commands, see the [README](https://github.com/lovenityjade/zebesproject#build-from-source-linux-development).

# Linux AppImage

ALPHA-0.24 provides `The_Zebes_Project-ALPHA-0.24-x86_64.AppImage`.
See [release validation](README.md) and `linux-package-verification.json` for
runtime evidence; building an image alone does not count as a runtime test.

## Running

Make the file executable and launch it. A Vulkan driver and glibc 2.35+ are
required. If FUSE is unavailable, use `--appimage-extract-and-run`. The regular
mounted launch and an extracted AppRun were tested on gaming-pc / RTX 3060.

Persistent data lives at `$XDG_DATA_HOME/zebesproject`, falling back to
`$HOME/.local/share/zebesproject` if XDG_DATA_HOME is absent or relative. The ROM
is copied into `roms/`; Unreal settings, logs and save banks live under
`Unreal/Saved/`. Replacing the AppImage does not replace that data. Development
profiles are not silently imported or moved.

Every startup validates the player's ROM before initializing native gameplay.
The image provides neither a ROM nor optional soundtrack recordings.

## Packaging

Cook a fresh package using `Scripts/package.sh`; do not wrap a development
folder or a previous dirty stage. `Scripts/stage-runtime.py` copies only the
reviewed runtime subset. `Scripts/build-appimage.py` runs the source asset gate
and rejects known private/stale staged files. It has no bypass option.

Supply a relocatable CPython 3.11 prefix, including its standard library,
libpython, dependencies and notices. AppRun selects it explicitly, isolates
user Python paths and refuses a missing bundled runtime. The native bridge may
reuse that same interpreter after a reload, but rejects a different loaded one.

Use checksum-verified local appimagetool and type-2 runtime files. The builder
never downloads a moving runtime automatically, uses two compression workers,
includes engine/dependency notices and refuses to overwrite an existing image.

```sh
python3 Scripts/build-appimage.py --check-only --package /path/to/clean/Linux
python3 Scripts/build-appimage.py \
  --package /path/to/clean/Linux \
  --python /path/to/verified/Python \
  --engine-notices /path/to/Unreal/Engine/Source/ThirdParty/Licenses \
  --appimagetool /path/to/verified/appimagetool \
  --runtime-file /path/to/verified/runtime-x86_64
```

Outputs: the AppImage, `.sha256`, and `.build.json` payload/tool inventory. The
builder's `packagedGameTested: false` is intentional; keep a separate runtime
verification report for the exact image hash after testing it.

`Scripts/test-appimage-launcher.py` is a recording-stub regression for read-only
launching, spaces/non-ASCII paths, relocation, persistent storage and private-file
rejection. It does not render the game. `Scripts/test-python-runtime-reload.py`
tests bundled interpreter reuse across preflight and deterministic generation.

AppImage runtime licensing and pinned source/rebuild links are included in
[the AppImage notices](../../../Licenses/AppImage-source.md).
