# ALPHA-0.24 Windows randomizer menu crash

Fixed on 2026-09-16. Ending-sequence work remains paused and is not part of this Windows hotfix.

## Cause

Opening the randomizer settings starts an asynchronous compatibility check.
Both the HUD and the randomizer used `SMNativeLibrary::Open`, then released
their respective handles through `FPlatformProcess::FreeDllHandle`.

In Unreal 5.8.2, `FWindowsPlatformProcess::LoadLibraryWithSearchPaths` returns
`GetModuleHandle` when the DLL is already loaded. That lookup does **not**
increment the Windows module reference count. The validator therefore received
a borrowed handle and subsequently unloaded the HUD's native library. Calls
through cached native function pointers then caused an access violation.

`SMNativeLibrary::Open` now uses `LoadLibraryExW` on Windows, with an absolute
path and the DLL-directory/default dependency search flags. Each caller owns
a reference, matched by its existing release. Linux's loader remains unchanged.
Windows headers are confined to the implementation file, avoiding macro
collisions with native rendering headers.

The missing `aqProf.dll`, `VtuneApi.dll`, `VtuneApi32e.dll` and
`WinPixGpuCapturer.dll` messages are optional profiling/capture probes. They
were not the cause of this crash. A missing crash-report client was a secondary
diagnostic limitation after the failure, not its trigger.

## Evidence

- Reproduced the crash by opening the Randomizer page in the original published
  Windows executable under Proton Experimental on gaming-pc.
- Added `-SMNativeLifetimeSelfTest=<isolated-root>` to the development build.
  On actual Windows (`sekailink-windows`), `-SMLegacyDllLifetime` reproduces the
  ownership failure safely: `Temporary release unloaded the live HUD module`.
- The corrected loader passes on actual Windows: four temporary open/release
  cycles and four asynchronous settings validations preserve the owner's
  callable frame export. Releasing the final owner unloads the module, verifying
  balanced references rather than permanently pinning the DLL.
- Windows Development executable built successfully with MSVC and Unreal 5.8.2,
  using two compilation workers. The native DLL and cooked content were unchanged.
- The corrected Windows executable renders the Randomizer menu through D3D11 /
  DXVK on gaming-pc, displays **Settings compatible**, survives the capture and
  exits normally. The screenshot was visually inspected.
- See `verification.json` for executable hashes and test outcomes. Raw logs and
  the screenshot are retained locally beside this document (ignored by Git).

These are targeted loader/settings-menu regressions, not a new full campaign
playthrough or a complete retest of every randomizer feature. The previous
release tests covered gameplay and standalone randomizer calls but missed
temporary library releases while a live HUD retained function pointers.

## Applying the prepared hotfix

Close the game, extract the Windows Randomizer Hotfix ZIP into the existing
ALPHA-0.24 installation, and allow replacement of:

`SMUnreal/Binaries/Win64/SMUnreal.exe`

The archive contains that executable and installation instructions only. It
does not contain a ROM, replacement native DLL, game assets or saved profiles.
It leaves the original root launcher and all saves/settings in place.
Do not put the inner executable beside the root launcher: preserve the folders.

The hotfix archive is prepared locally; publication is separate. Public source
cleanup (no top-level Scripts/Tests) and the paused ending changes must remain
separate when transferring this fix.
