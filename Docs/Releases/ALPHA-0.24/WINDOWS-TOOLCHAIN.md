# Windows toolchain — September 16, 2026

## Scope

Machine: `sekailink-windows`. Install the Windows engine needed for the frozen
ALPHA-0.24 release work; do not change accepted Linux gameplay or player saves.
macOS is excluded. This record is not evidence of a working Windows game build.

## Launcher recovery

The user signed into Epic Games Launcher, but the Unreal library initially had
no available engine versions and its installation control did not work.
Opening the game library exposed a pending Epic terms screen. After completing
that step and restarting the launcher, its engine catalog populated. The initial
5.8.0 placeholder refreshed to **5.8.2**. The connected session was retained.

The requested installation was started through the official launcher at
`D:\Epic Games\UE_5.8`, with the core engine and engine source. Templates,
debugging symbols and MetaHuman content were not selected. No additional game
was installed. The directory name is the launcher's minor-version convention;
the selected build is `5.8.2-56702186+++UE5+Release-5.8-Windows`.

**Current state: installation completed; native DLL, Unreal Game and Editor
module compile. Native and Unreal headless checks passed; packaged validation
and distribution clearance remain outstanding.**

The deployed `Build.version` reports 5.8.2, changelist 56702186 and compatible
changelist 55116800, matching the existing Linux engine. The editor executable
and build scripts are present.

## Completed verification

- At 12:06 EDT, the launcher displayed **Installed** and enabled **Launch** for
  Unreal Engine 5.8.2. Its log recorded `AlertCode=[ok]`, `IncompleteInstall=0`
  and a committed installation manifest.
- The engine's `.item` record under
  `C:\ProgramData\Epic\EpicGamesLauncher\Data\Manifests` confirms the exact
  build, installation path and `bIsIncompleteInstall: false`.
- This launcher version left the legacy `LauncherInstalled.dat` list empty.
  Use the completed `.item` manifest and installer result, not that legacy
  index alone, when checking this machine.
- At 12:08 EDT, UnrealBuildTool ran using the engine's bundled .NET runtime:
  `-Mode=ValidatePlatforms -Platforms=Win64 -OutputSDKs -NoMutex`.
  It exited successfully and reported **`Win64 VALID`**.
- [Machine-readable verification](windows-engine-verification.json) records
  the exact version and result. **The game itself has not been built/tested on
  Windows by this setup step.**
- The two temporary diagnostic/launcher scheduled tasks were removed after they
  finished. The launcher remains connected; no recurring setup task was left.

## Existing compiler tools

- Visual Studio 2022 Build Tools:
  `C:\Program Files (x86)\Microsoft Visual Studio\2022\BuildTools`.
- MSVC toolset folder: `14.44.35207`; actual `cl.exe` product version:
  **14.44.35228.0**. These numbers differ; the executable version is above the
  `14.44.35210` ban in UE 5.8.2's `Windows_SDK.json`.
- Windows SDKs: **10.0.19041.0**, **10.0.26100.0**.
- .NET Framework **4.8 SDK** (`Microsoft.Net.Component.4.8.SDK`) was added through
  the existing Visual Studio Installer for the Editor's SwarmInterface build
  dependency. Installer exited 0; the SDK is registered under
  `C:\Program Files (x86)\Windows Kits\NETFXSDK\4.8\`. No reboot was performed.
- MSYS2 UCRT64 GCC, CMake, Python and `libwinpthread-1.dll` are present.
  Presence alone does not verify native-library compatibility.
- Machine inventory: 8 logical processors, 32 GiB RAM, Radeon RX 580.

## Native Windows port verification

The release preparation branch now builds `sm_native.dll` with UCRT64 GCC
16.1.0. Unreal loads that same DLL for gameplay, generation and tracker tests.
Windows uses SRW locking, a monotonic performance counter and a local embedded
CPython runtime. UTF-8 file paths go through wide Windows APIs; save commits
replace an existing destination without first deleting the previous save.

`Scripts/test-windows-native.py` passed on `sekailink-windows` in
`D:\ZebesBuildSetup\ALPHA024`, with the required isolation marker:

- All **198 declared public native functions** resolve from the DLL.
- Three seeds, **14092026–14092028**, each have 100 accessible checks and a
  verified completion route. Each progression log agrees with its item placements.
- Empty/full inventories produce the expected tracker availability changes;
  all 100 checks and 10 boss entries are returned.
- A repeated seed after other profiles retains its fingerprint. Invalid settings
  are refused without leaking into the next request.
- A separate C executable loads the bundled interpreter with only Windows
  System32 on PATH; MSYS2 and installed Python are not runtime prerequisites.
- Missing and invalid-CRC ROMs are rejected. A valid ROM and save/load work in
  `Windows port é 漢字`; a second save replaces the first successfully.
- 360 real native frames execute with **zero emulated 65816 opcodes**. Source ROM
  bytes are unchanged. This is a native-core test, not a rendered Unreal test.

See [machine-readable native results](windows-native-verification.json).
The DLL imports only Windows/UCRT system libraries, not `libwinpthread` or
`libgcc`. CPython's own runtime DLLs remain beside its interpreter.

The bundled runtime is the official [CPython 3.13.15 embedded x64 package](https://www.python.org/ftp/python/3.13.15/python-3.13.15-embed-amd64.zip),
SHA-256 `d1f04d990aee1253d8569e8e5104e30fa9f5fa830899f14843448872d936a2cf`.
`Scripts/prepare-windows-python.ps1` checks that digest and leaves the isolated
`._pth` configuration intact. Its complete `LICENSE.txt` must accompany a release.
The editor reuses its already loaded Python 3.11 interpreter. Its tracker test
passed separately in UnrealEditor game mode.

## Unreal host verification

Both `SMUnreal Win64 Development` and `SMUnrealEditor Win64 Development` compile
with MSVC 14.44.35228 and Windows SDK 10.0.26100.0. The full settings catalog is
byte-for-byte unchanged; its generated C++ representation is split into small
raw strings because MSVC rejects the original 126,911-byte literal. Windows
profile/SRAM publication uses replacement semantics, preserving the old file
until its replacement is committed. Linux retains its original rename behavior.

Five actual UnrealEditor game-mode checks passed with `-nullrhi -nosound`:

- Compatible ROM validation/import and rejection cases.
- Settings-string round trips, including the complete offline catalog.
- A/B/C independence, repeated writes, seed resolution, copy/import and recovery
  of an interrupted bank transaction.
- Native asynchronous tracker/controller integration across Vanilla and seeds.
- Native Vanilla/Story/Boss Rush/Randomizer menu flow and the settings entry point.

See [Unreal host results](windows-unreal-verification.json).
These headless checks do not establish rendered output or audio. A raw uncooked Game
launch stopped during Unreal asset loading before reaching the tests; the
successful checks use the editor host. A clean cooked/package launch is still a
separate release gate, not covered by successful compilation.

The separate **D3D11/SM5 rendered startup test passed at 1280 × 720** on the
Radeon RX 580 (8 GiB dedicated video memory). It exercised input-skipped warnings,
creator logo, the native 2026 glyphs, title-camera phases/fades, ready/Press Start
and save selection. Eight screenshots were produced; the warning, ready title
and save selection captures were retrieved for inspection. The warning and title
were visually inspected successfully. **The save-selection screenshot is black
except for the version label.** Delaying capture by four ticks and shutdown by
eight ticks did not resolve it on a second run. The native state-machine PASS
does not validate that screen's rendering; this remains an open Windows issue.
See [rendered test record](unreal-windows-Title.json).

The first run spent several minutes compiling uncached shaders with one CPU
worker before the timed presentation could proceed; D3D11 device creation itself
succeeded. This is not evidence of GPU exhaustion. No full gameplay frame-rate,
worst-case combat effects or audio assessment is claimed by the startup test.
The temporary interactive render-test scheduled task was removed after success.

Development commands from a Windows PowerShell prompt:

```powershell
.\Scripts\prepare-windows-python.ps1
.\Scripts\build-windows.ps1 -NativeOnly
.\Runtime\Python\python.exe -I .\Scripts\test-windows-native.py D:\ZebesBuildSetup\ALPHA024
.\Scripts\build-windows.ps1 -GameOnly
```

The test requires a private compatible ROM in the isolated tree's `roms` folder.
Do not point it at an installed player profile or add that ROM to an archive.

## Remaining release work

1. Complete the Unreal Windows build and rendered validation; native-core tests
   above do not establish Unreal rendering, controller or audio behavior.
2. Validate the Windows game in isolation from player profiles, then
   perform packaging and ROM-gate tests. Asset distribution gates still apply.

## Portability findings from the frozen source (baseline)

| Area | Current implementation | Required Windows work |
| --- | --- | --- |
| Native build | `Native/CMakeLists.txt` invokes `python3`, uses GCC flags, ELF linker flags and `m dl pthread` | Select the supported compiler/Python explicitly, set the DLL name, make system libraries platform-specific and preserve the `cpu_runOpcode` native-only guard |
| Public native ABI | `Native/sm_bridge.h` uses ELF visibility attributes | Export the required C entry points from the Windows DLL and validate the exported symbol list |
| Randomizer interpreter | `Native/sm_randomizer.c` uses `dlfcn`, pthread locking and Linux `libpython*.so.1.0` names | Use the Windows library loader and locking, provide a compatible CPython runtime/standard library, and test generation, preflight and live tracking in the packaged build |
| Profiling clock | `Native/sm_profile.h` assumes `clock_gettime(CLOCK_MONOTONIC)` | Supply a supported monotonic Windows implementation without changing simulation timing |
| Unreal native loading | `SMHUD.cpp`, `SMRandomizer.cpp` and `SMTrackerSelfTest.cpp` hardcode `Native/build/libsm_native.so` | Share a platform-aware library path across gameplay, generation and tests |
| Packaging | `Scripts/package.sh` and `Scripts/stage-runtime.sh` select Linux paths and a Bash launcher | Add a Windows package path/launcher, include needed runtime DLLs and licenses, and retain the distribution gate |

The MSYS2 tools found on this development machine must not become an undocumented
requirement on a player's computer. Validate a packaged installation without
system Python/MSYS2 on PATH, including paths containing spaces and non-ASCII
characters. `fopen` calls in native code require particular attention for UTF-8
paths on Windows. Keep the ROM gate and per-slot save/seed isolation in the
Windows test scope; a successful engine install proves none of these behaviors.

Temporary diagnostic scripts and captures are outside the committed source,
under `.tmp/release-alpha024` locally and `D:\ZebesBuildSetup` on Windows.
No account credentials or authentication tokens belong in this record.
