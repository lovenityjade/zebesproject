> **Public source layout:** development scripts and test harnesses referenced below
> are retained in the private development repository. For current public build
> commands, see the [README](https://github.com/lovenityjade/zebesproject#build-from-source-linux-development).

# Development guide

See the [project README](../README.md) for current features, credits, license
terms, ROM requirements and platform status. ALPHA-0.24 gameplay/presentation
is frozen; release preparation must not silently change that accepted baseline.

## Local source setup

Initialize the three pinned Git submodules and set
`git config core.hooksPath .githooks`. `SM_ENGINE` points to a separately installed
Unreal Engine 5.8.2. Linux scripts use GCC, CMake, Python 3,
a shared CPython runtime and Pillow for asset preparation. The lighting bake
currently needs the validated ROM at `roms/Super Metroid (Japan, USA) (En,Ja).sfc`
before a source build. Never add that file to Git.

- `Scripts/build.sh`: native library and Unreal Editor game module.
- `Scripts/play.sh`: the normal local `UnrealEditor -game` launch path.
- `Scripts/package.sh`: Linux package pipeline, currently stopped by the mandatory
  distribution audit. Do not bypass its guard or the pre-push hook.
- `Scripts/stage-runtime.sh`: runtime staging, with ROM/private-save exclusions.
- `Scripts/build-windows.ps1`: Windows native DLL (MinGW-w64/UCRT) and Unreal
  Game/Editor module (MSVC). `-NativeOnly` and `-GameOnly` select build stages;
  `-Target SMUnrealEditor` selects the editor module.
- `Scripts/prepare-windows-python.ps1`: verify and stage the isolated, pinned
  CPython embedded runtime under ignored `Runtime/Python`.
- `Scripts/test-windows-native.py` and `Scripts/test-windows-unreal.ps1`:
  isolated Windows validation; see [toolchain/results](Releases/ALPHA-0.24/WINDOWS-TOOLCHAIN.md).
- `Lancer-Editeur-Decors.sh`: external decoration editor;
  [guide](RoomEditor/README.md).

Do compilation, automated playtesting and image preparation on the designated
build/test hosts: gaming-pc for Linux and sekailink-windows for Windows. A local launch is for the player's acceptance
run. Test fixtures must use isolated directories and independent SRAM/profiles.
Never overwrite an accepted build while it is running; stage new files and retain
its previous binaries. Do not migrate the accepted installation to an untested
release-port branch.

## Useful documentation

- [Native menu and per-slot start flow](MENU-START-FLOW.md)
- [Startup, title assets and native music timing](STARTUP-TITLE.md)
- [System settings](SYSTEM-MENU.md)
- [Randomizer integration](../Randomizer/README.md)
- [Quality of life](QUALITY-OF-LIFE.md)
- [ROM gate and distribution blockers](ROM-SETUP.md)
- [ALPHA-0.24 validation](ALPHA-0.24-VALIDATION.md)

Root `VERSION`, Unreal `ProjectVersion`, release notes and `CHANGELOG.md` must agree.
The version label is compiled into the front end. Each changed runtime build needs
appropriate regression checks; a successful build alone is not a gameplay proof.
