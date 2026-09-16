# Optional save-station travel — work checkpoint

Status: **PAUSED at the user’s request, 2026-09-15. Implementation incomplete.**

## Requested behavior

At loading an existing randomized save, reuse the original Planet Zebes region selector and original room map to select a previously unlocked save station. This is an optional randomizer feature, disabled by default. Keep the original game assets and native menu transitions. Settings and unlocked destinations must stay independent for A/B/C and their seeds. Support enabling this option on an existing randomized slot.

The implementation direction communicated to the user is to treat a station as unlocked when the player has saved there, using the existing SRAM station flags. Revealing all randomized maps must not unlock destinations. Each station naturally covers its local portion of a region; no invented subregion artwork is planned.

## Edits already on disk (NOT built or tested)

- `Native/sm_travel.h` and `Native/sm_travel.c`: draft native implementation. Per-slot enable flags; randomized/ready gating; station mask from the lower byte of `used_save_stations_and_elevators`, excluding elevator bits, debug load stations and invalid ROM records. Includes the original last-saved station as a fallback when its valid station index is below 8. Original ship load point is represented by Crateria station 0.
- Draft original-area-menu input replacement: D-pad selects regions with unlocked stations, A/Start opens a region, B returns through the original transition.
- Draft room-selection input: L/R or Select cycles unlocked stations using the original room-map initialization and blinking marker; D-pad scrolling and original B/A/Start behavior remain in the upstream routine. Destination is validated before confirmation. No SRAM write is added to browsing/loading.
- Draft native station icons, region station counts and English control hints rendered with the existing original map font.
- `Native/sm_map_browser.c/.h`: original text renderer exported as `sm_native_map_text` for reuse; existing browser text calls renamed accordingly.
- `Native/sm_generation.c`: resets travel flags through `sm_slots_reset`.
- `Native/sm_bridge.c`: invokes travel text rendering after map-browser rendering.
- `Native/prepare_overlays.py`: includes travel header in generated banks, hooks original area/room selection input, filters area labels to unlocked destinations while travel is enabled, draws unlocked station icons. Changes are in the overlay generator, not upstream source.
- `Native/CMakeLists.txt`: includes `sm_travel.c`.

No Unreal profile/settings/UI changes have been made yet. Nothing currently calls `sm_travel_configure` from Unreal, so this draft feature is not usable from the menu yet. Do not report it as delivered.

## Remaining work

1. Add an optional persisted setting to randomizer slot metadata with a false default for existing profiles. Preserve it through pending generation, completion, slot copy, import and reset; ensure replacing a slot cannot inherit another seed’s preference accidentally.
2. Expose an English randomizer setting (World or Quality of Life page), including a clearly identified active-slot toggle for an existing seed. Wire the native per-slot API through `FSMSystemMenu::UpdateNativeSlot` and reconfiguration. Keep seed generation settings immutable after generation.
3. Review native travel validation and original menu integration. Pay particular attention to cancel/re-entry, repeat input, cursor centering, the last-save fallback, original ship semantics, and keeping exploration/SRAM intact while browsing.
4. Compile native and Unreal changes, using a separate native build while the local game is running. Stage only after successful validation.
5. Run automated runtime tests **on gaming-pc**, in an isolated marked test directory, preserving the real player’s saves. Test enabled/disabled/vanilla, multiple unlocked stations and regions, single-station/empty regions, elevator-only flags, full-map independence, A/B/C isolation, actual load destination and safe position, cancellation, and byte-identical SRAM before/after browsing/loading. Verify original native rendering with captures. Add profile persistence/copy/generation coverage.
6. Re-run relevant native region-browser and pause regressions after changes.
7. Document verified behavior, controls, remaining limitations and artifacts. Do not launch/restart the local game without a new request.

## Current runtime and saved-game safety

The user’s local game was launched earlier in this session via `Scripts/play.sh` and was still running at the last check (PID 528022). Log: `.tmp/local-play-map-browser.log`; native readiness marker observed. This running build has the completed pause region browser, not this unfinished travel feature. Recheck the process before any future runtime action.

No compilation, staging, travel test, save modification, process termination or restart has been performed during this travel work. Preserve the prior Morph Ball save repair and its recovery backup under `Unreal/Saved/SM/Recovery/20260915-hidden-morph`.

Existing remote isolated test root from previous completed work: `/tmp/sm-native-generation-20260914` on gaming-pc. Existing fixture helper: `Scripts/test-live-tracker-native.py`; existing region-browser validation: `Scripts/test-map-browser-native.py`. The fixture helper defaults to native slot B and skips the original load map; a travel test must explicitly exercise the genuine load-menu path instead.

## Separate visual request to retain

User request, 2026-09-15: make Speed Booster more electrified and add light flashes on objects/enemies it destroys while passing through them. **Not implemented; follow-up visual work.** Keep original sprites and the project’s established scene-lighting approach.

The interim request to record an intermittent sound-loss bug was explicitly withdrawn by the user; it is not an active investigation or backlog task.
