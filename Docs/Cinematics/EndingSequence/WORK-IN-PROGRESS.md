# Ending sequence — paused checkpoint

Date: 2026-09-16. **Paused at the user's request: a critical Windows bug takes priority.** Its symptoms and reproduction steps have not yet been provided. Do not resume ending work or its tests without the user's instruction.

## Requested behavior

Restore the native escape from Zebes and planet destruction between gunship departure and credits, including the saved-animals variant. Enhance the existing artwork with the established cinematic lighting. Make the animals' small ship a larger blue light with apparent volume, halo and trail. Add **View Ending Sequence** to Debug.

## Implemented, not released

- Removed the Chozo Tablets shortcut in `Native/prepare_overlays.py` that selected `CinematicFunction_Intro_Func126` (credits) instead of `CinematicFunctionEscapeFromCebes`. Also removed its now-redundant credits music requeue. Native scene timing and the later Samus ending remain in control.
- `Native/sm_cinematics.c` and `Shaders/Cinematics.usf`: warm planet rim/explosion wave; shaded blue sphere, halo and particles anchored to the actual `8B:EF21` animal ship. The native rescue flag is event 15. Contrary to the remembered direction, native `CinematicFunction_Intro_Func151` moves it **right**, from x=128 toward x=272; this trajectory is preserved.
- New `Native/sm_ending.c`, `.h`, and `sm_ending_runtime.inc`: in-memory preview with `Current save`, `Not rescued`, and `Rescued` variants; Start cancels. Restore the native runtime before entering the existing isolated credits preview, so the real run is not marked completed.
- `Native/CMakeLists.txt` appends the runtime seam to a generated copy of upstream `sm_rtl.c`; upstream remains untouched. Runtime snapshot includes RAM/SRAM, hardware/render state, SPC/DSP and queued audio. Preview frames bypass the gameplay input recorder. Native save is suppressed during preview; shutdown restores first. Optional remastered audio state is left untouched; preview uses native SPC audio.
- `sm_bridge.c`, `sm_credits.c`, `sm_soundtrack.c` integrate preview handling.
- `SMSystemMenu.h/.cpp`: Debug button **View Ending Sequence**, animal selector, return button. Enabled from stable gameplay or native pause; existing credits preview remains available.
- `SMHUD.cpp`: cinematic fixture case 3 now invokes the preview and accepts `-SMEndingAnimals=-1|0|1`; test warmup navigates the redesigned mode row to Start Game.
- Private development script `Scripts/test-ending-sequence.py` covers actual native ending traversal, preview restoration/re-entry/cancel, both rescue states, and a real tablet-triggered ship departure. `--tablet-only` runs only that additional regression.

## Verified on gaming-pc

- Native C library build passed; Unreal Editor target build passed with two workers.
- Both native rescue variants traversed phases 5 → 6 → 7 → credits. The animal light exists only in the rescued variant (143 frames, x=129 through x=271).
- Full debug preview returns to identical 128 KiB native RAM and the original gameplay frame counter. Repeated launch and early cancellation pass. A snapshot buffer capacity-reset bug found during testing was fixed.
- A randomized tablet fixture reaches the quota, acknowledges the warning, starts the countdown, enters the ship, traverses the restored cinematic and reaches credits; escape timer stops. No emulated CPU instructions.
- Source ROM hash unchanged. Fixture saves are copies in an isolated SMTests directory; player saves were not edited.
- Reports preserved here: `native-verification.json` and `tablet-verification.json`.
- Material regeneration commandlet succeeded and the saved material contains the new sphere shader.

## NOT yet verified / next steps when resumed

1. **Visual acceptance is pending.** Unreal/Vulkan capture run was stopped at the user's request after the first `original` capture marker. No complete before/after review, shader appearance approval or complete rendered pass may be claimed.
2. Review planet rim/explosion and animals' sphere/trail with effects on/off, then inspect the Debug menu and exercise it interactively.
3. Additional preview coverage worth checking: native pause, exact SRAM/sidecar and audio continuation, shutdown during preview. Current successful snapshot assertions cover RAM and frame counter, not all of these separately.
4. Review startup audio/cleanup against native state 38, and Windows compilation/export compatibility before distribution.
5. No local build, local launch, new release, public push or public source synchronization has been done for this work. Changes are uncommitted in the private development checkout.

## Reproduction environment and boundaries

- Development checkout: `/home/thelovenityjade/Projects/projectSM`, branch `release/alpha-0.24-portability`; origin is the private development archive.
- Public checkout: `Build/PublicSource-ALPHA024`, main at `f0f7c79` at this checkpoint. Public Scripts/Tests were intentionally removed. If later transferring this patch, preserve public CMake preprocessing paths under Native and do not copy the entire private tree or publish private history.
- Isolated remote tree: `gaming-pc:/tmp/zebes-alpha024-release-20260916`, containing `ISOLATED_TEST_DIRECTORY`.
- Engine: `/home/nobara-user/Applications/UnrealEngine-5.8.2`.
- Native artifacts: `Native/build/libsm_native.so`; UE editor module: `Unreal/Binaries/Linux/libUnrealEditor-SMUnreal.so`; regenerated material: `Unreal/Content/SM/M_Present.uasset` (remote only).
- Original read-only fixture: `/home/nobara-user/Games/SuperMetroid-VisualValidation/Unreal/Saved/SMPreview/sram.dat`. Test saves are copied into `ending-results/SMTests` or Unreal's `Saved/SMTests`.
- Remote reports/fixtures: `ending-results/`; tablet seed copied from `Docs/Randomizer/FullOptions/ElevatorsHud/Seeds/seed-4.json` to `ending-results/relic-seed.json`.
- Remote logs: `/tmp/zebes-ending-build.log`, `/tmp/zebes-ending-test.log`, `/tmp/zebes-ending-tablet-test.log`, `/tmp/zebes-ending-unreal-build.log`, `/tmp/zebes-ending-material.log`, `/tmp/zebes-ending-visual.log`.
- Last render command: `UnrealEditor <project> /Engine/Maps/Entry -game -SMCinemaTest -SMCinemaCase=3 -SMEndingAnimals=1 -SMCinemaSample=1745 -SMTestSavedRoom -SMTestSave=<fixture> -SMWarmup=8500 -RenderOffscreen -unattended -windowed -ResX=1280 -ResY=720 -nosplash -NoZenService -nosound -ExecCmds="t.MaxFPS 60"`.
- The ending test and its shader workers were explicitly stopped for this pause. Do not restart them while addressing Windows.
