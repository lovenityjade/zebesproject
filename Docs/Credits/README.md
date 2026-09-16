# The Zebes Project — credits and run statistics

Native credit roll and Unreal atmosphere integration, 2026-09-15.

## Order and original artwork

1. **The Zebes Project**, **TheLovenityJade**, **Sekailink**, **sekailink.com**.
2. Native decompilation: **snesrev**, **DaBanana64**, **Lywx**; native runtime attribution: **elzo_d**. Disassembly references: **strager / Matthew Glazar**, **Blake Smith**, anonymous contributors, **PJBoy**, **Kejardon**.
3. The original VARIA credit sequence, including its preserved Nintendo / original Super Metroid staff section and VARIA contributors. The code reads the pinned upstream script's actual display order, not the storage order of its text table. Includes Dude and Flo, Total, Dessyreqt, contributors, community thanks, tools and URLs.
4. Gameplay statistics, followed by **Thanks for Playing**. A completed game then continues into the original ending sequence with Samus.

Names use the original uppercase pixel font. VARIA's colored section titles and two-row name/numeral glyphs come from the original game. No image generation or substitute typeface is used.

The backdrop is the **actual Zebes approach scene** used by `CinematicFunction_Intro_Func86`, decoded from ROM tiles `96:EC76`, tilemap `97:8ADB` and its original palette. The planet is translated intact and dimmed behind the credits; native star pixels twinkle and sparse stars extend the sky into the wide margins. The Unreal presentation shader adds a smooth amber atmospheric rim and blue-white star glints at output resolution. It uses the existing Gaussian layer and preserves the GUI's exact glyph colors. Atmosphere can still be disabled from Settings.

`Scripts/build-credits-assets.py` regenerates `Native/sm_credits_assets.inc` from the original verified ROM and pinned local VARIA sources. Normal builds and runtime consume the compiled include. No external tracker pack, website request or generated art is involved. [Asset provenance](assets.json).

## Debug preview

Open the system menu, select **Debug → Launch Credit Roll**. It is available during gameplay or the native pause screen, outside item messages. It previews the selected slot's current statistics.

- **Start** returns immediately to the original session.
- Hold **Right** to scroll eight times faster.
- Letting the roll finish also returns automatically.
- **Debug → Return from Credit Roll** provides another way back.

The preview suspends native simulation and uses its own native SPC music player. It does not change game RAM, SRAM, the native frame clock, seed flags, completion flags or statistics. Gameplay audio state is retained. Presentation buffers are restored on exit; the real ending is not triggered by the preview.

## Statistics and persistence

`Native/sm_run_stats.c` maintains three independent records beside each SRAM file in `<sram-path>.stats`, with a versioned header, checksum and temporary-file rename. Records are bound to slot and seed fingerprint, or Vanilla identity. Successful native slot copy/clear/regeneration actions copy/reset the corresponding record. This sidecar is separate from the existing SRAM/seed transaction journal; collection/equipment saves retain their original format.

Recorded fields correspond to VARIA's credit statistics:

- Active-session real time and the original in-game clock, formatted as hours/minutes/seconds/native frames.
- Door transitions, time in door transitions, alignment frames and native pause-menu time.
- Time in all twelve VARIA graph regions, excluding time in the pause menu.
- Uncharged beams (including Hyper), charged beams, special beam attacks, missiles, supers, bombs and Power Bombs.
- Deaths, saved-game restarts and native missed-NMI time outside item messages.

Shot counts hook successful native firing paths, not input presses or arbitrary ammo decreases. A special beam attack counts once, not as a Power Bomb. Perfectly aligned doors add zero alignment frames. The run freezes on the final Down press to board the ship after escape, excluding the takeoff animation. Native menus and a suspended desktop/system menu are not treated as active gameplay wall time. GPU slowness is not reported as SNES CPU lag.

Statistics are cumulative across saved-game restarts, including failed attempts. They save with the existing SRAM writes and at completion/death. Old saves cannot supply historical shots, deaths or region times: the roll marks incomplete records **Tracked since this update**. The in-game clock remains the original saved clock. A newly generated slot carries an explicit never-started marker through its automatic save and reload: its first Start Game begins a complete record at zero resets. The preview never invents demonstration totals.

The sidecar requires the same writable save directory as SRAM. If copying a save outside the in-game slot interface, keep its `.stats` file with it. This implementation does not import upstream VARIA SRAM extensions or backup-save formats.

## Implementation

- `Native/sm_credits.c`: authored introduction, copied upstream credit rows, live statistics, native glyph composition, reversible preview and dedicated preview music.
- `Native/sm_run_stats.c`: per-slot persistence, clocks and stat counters.
- `Native/prepare_overlays.py`: hooks the actual credits object and final boarding event in generated bank copies. The original cinematic animation and final Samus sequence keep running through the native engine.
- `Scripts/prepare-stats-movement.py`: exact-count hooks in successful native beam/ammo/bomb routines.
- `Unreal/Source/SMUnreal/SMSystemMenu.cpp`: English debug controls.
- `Shaders/Present.usf`: output-resolution halo and star glints, after the existing Gaussian pass and below the protected text.

Attribution follows the checked-out repositories' contributor history and VARIA's authored credits. Original Super Metroid artwork and Nintendo staff credits are preserved. See `Native/Credits-LICENSE.txt` and the existing runtime notices.

## Validation scope

All runtime tests use the marked isolated directory on **gaming-pc**, without modifying the installed game or user saves. Native preview tests compare complete RAM, SRAM, frame clock and statistics before/after automatic completion and Start cancellation. Native stat fixtures use real controls for beams, missiles, bombs and pause, then reload their sidecar and traverse the ending. A/B/C menu tests verify first-start statistics, copy and clear behavior. The ending fixture enters state 38, including the native fade and room HDMA cleanup; jumping directly to state 39 would incorrectly retain gameplay weather/blending. The Unreal offscreen test captures the real Vulkan material with atmosphere off/on and animated glints.

The evidence files in this directory identify completed checks and exact build hashes. Controlled ending entry is not a boss-fight playthrough, and the new statistics cannot recover activity from before their installation.

### Reproduction

Developer checks (no interactive game launch):

```sh
cmake -S Native -B Native/build
cmake --build Native/build -j2
python3 Scripts/build-credits-assets.py
python3 -m py_compile Scripts/test-credits-native.py Scripts/test-credits-stats.py Scripts/test-native-slots.py
```

The native test scripts take a marked isolated directory on gaming-pc. `test-credits-ending.py` captures intermediate Samus and final item-percentage scenes after the roll. `test-native-slots.py` creates its own A/B/C bank and must use a separate fixture directory. `test-credits-native.py` and `test-credits-stats.py` reuse the established native tracker fixture inputs; they never target user profiles.

The cooked Unreal build accepts `-SMCreditsTest -SMTestSavedRoom -SMTestSave=<isolated fixture> -SMWarmup=8500 -RenderOffscreen -unattended -ResX=1280 -ResY=720`. This produces six real Vulkan captures, GUI masks and `credits-unreal-verification.json` under `Saved/SMTests`. Run `Scripts/check-credits-captures.py <capture-directory>` to check exact text preservation, visible atmosphere and animated stars.

### Recorded evidence

- [Build and scope](build-evidence.json)
- [Native preview recovery](preview-verification.json)
- [Actual controls, persisted statistics and ending](stats-verification.json)
- [Independent slots, fresh statistics, copy and clear](slots-verification.json)
- [Unreal Vulkan preview](unreal-verification.json) and [pixel comparison](visual-verification.json)
- [Tracker regression](tracker-regression.json)
- [Zebes and project credits](credits-glow.png), [decompilation credits](credits-decompilation.png), [VARIA/original staff](credits-upstream.png), [statistics](credits-statistics.png)
- [Native Samus ending](post-credits-samus.png) and [following ending scene](post-credits-final.png)
