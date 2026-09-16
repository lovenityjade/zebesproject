# Native alternate starts — integration checkpoint, 2026-09-15

The generator, English settings, immutable per-slot seed, native new-game menu, save routing and live tracker now share a versioned start/world plan. **The full VARIA integration goal remains incomplete.** Area/boss/door randomization, mirror, Minimizer, Scavenger, general objectives, Tourian/escape/race and the remaining gameplay patches are still required.

## Supported behavior

All 15 pinned start definitions have native spawn/load handling. Thirteen are usable with the currently supported unshuffled world: Ceres, Landing Site, Gauntlet Top, Green Brinstar Elevator, Big Pink, Etecoons Supers, Wrecked Ship Main, Firefleas Top, Business Center, Bubble Mountain, Mama Turtle, Watering Hole and Aqueduct. VARIA requires area randomization for Red Brinstar Elevator and Golden Four; their native load fixtures pass, but their seed modes remain blocked by the unfinished area integration.

Each A/B/C slot owns its start and ordered world-data selection. **Generate Game** applies the plan and writes real initial SRAM before enabling Start Game. New slots store their actual region/station and loading mode, including Ceres. The immediate first Start still uses the new-game path; subsequent loads use the saved station. A later save at the ship does not reset to the seed's original start. Failed publication restores SRAM, all native RAM and the previous world selection.

The seven custom save starts install VARIA's original load records, music/map-icon data and original save-station PLMs with the correct saved station index. Room installation is idempotent. Non-ship starts wake Zebes and receive their specified initial door bits. They no longer inherit the old unconditional Landing Site blue door. Watering Hole's door $A498 runs the exact C equivalent of `wh_open_tube.ips`: set event $0B, without writing its new 65816 routine into the ROM.

The generator captures actual forced settings from VARIA's patcher settings: start, split, Morph placement, suit restriction, HUD and layout/tweak selections. Informational forced-option messages are retained as generation notes when a full placement and solver proof exist. They no longer incorrectly turn a successful seed into a generation failure. FullWithHUD/alternate Major starts use VARIA's effective HUD choice. Random starts are resolved by VARIA against both the user's allowed list and the eligible skill/Morph/world context; the resulting start is recorded. Unsupported or invalid requests still fail.

The live tracker evaluates availability and return paths from the effective start. Ceres uses Landing Site as its return destination because the prologue cannot be revisited from Zebes. Relic Hunt still requires returning to the ship, not to the initial station. No seed placement enters the live accessibility query.

## Reversible world data

`Scripts/audit-native-starts.py` records 15 starts and 44 source patch definitions in `dependencies.json` and `patch-spans.md`. `Scripts/build-world-data-catalog.py` compiles 40 reviewed data-only patches, 85 spans and 65,017 backup bytes into `Native/sm_world_data.inc` and `Randomizer/native_world_data.json`. `Scripts/build-start-catalog.py` generates the immutable spawn/save-PLM descriptors.

The runtime accepts compiled IDs and a catalog fingerprint, not caller-provided ROM bytes. It restores original spans before applying the selected **ordered** list, then applies the randomized item table. Vanilla selection restores the initialized original world. Overlapping compressed-room patches follow VARIA's order rather than numeric bit order.

`Randomizer/world_catalog_history/vanilla-starts-v1.json` freezes the first released data catalog. Its IDs and data definitions are append-only; generation of a future catalog asserts that archived entries are unchanged. The native compatibility list retains matching historical catalogs when new definitions are appended, preventing future additions from reinterpreting saved seeds.

Executable patches remain excluded from the data layer. Watering Hole now has its separate C implementation. The Lower Norfair Chozo bypass and Bomb Torizo changes are now implemented in the subsequent [Tweaks increment](../Tweaks/README.md). Door indicator room integration remains unfinished and rejected; its subsequent [data audit](../DoorIndicators/README.md) shows that the indicator blob itself is PLM data interpreted by existing native routines. Accepting a typed custom patch list does not imply every member is implemented. Unknown and duplicate custom patch names are rejected even with the parent setting off.

## Verification

Runtime execution stayed in the marked isolated directory on **gaming-pc**. No local game was launched/restarted during this work, no actual user SRAM was edited, and the frozen Save Refill release hashes remain unchanged.

- `native-starts-verification.json` and `NativeCaptures/`: all 15 starts load and advance native frames, resolve the expected room/area/station, expose correct custom save PLMs/arguments without duplicates, retain the start through native SRAM save/reload, and preserve the source ROM. Nonstandard starts do not inherit the Landing Site door. These use a controlled existing placement plan, not generated route claims.
- `Seeds/` and `seeds-verification.json`: thirteen actual generated seeds, each with all 100 checks reachable and a verified completion route. Spoiler items agree with fresh progression logs. Fixtures include forced Morph/HUD/layout settings.
- `tracker-verification.json`: 65 in-process live queries across progression inventories for those 13 seeds, with explicit effective start/return-root checks and difficulty/technique checks for green markers. Native plans apply, restore Vanilla and reapply their world data.
- `random-options.json`: three deterministic random-start seeds, repeated fingerprints, area-only starts excluded from eligible choices, invalid-only lists and No Advanced Techs-incompatible Firefleas requests rejected.
- `bank-gauntlet-mama.json`, `bank-ceres-aqueduct.json`, `bank-elevators.json`: three native menu banks with independent B/C starts. They exercise generation gating, initial autosave, publication failure/RAM rollback, real reload, copy/clear, historical Vanilla restoration, and unchanged statistics behavior. Later-station reload uses a controlled native save at the ship; it is not claimed to be a walk to the ship.
- `data-verification.json`: 44 full-image transaction cases including reverse patch order, rejected-plan rollback and duplicate-order rejection. The reference is the initialized Vanilla image, which includes the decompiled core's pre-existing ROM fixes; the source file remains unchanged.
- `contract-verification.json`: catalog identity, invalid start/slot/missing save data and duplicate/unsupported IDs, invalid custom names and unimplemented-option rejection.
- `profile.log`: Unreal parses, publishes and reloads all 13 start plans with their ordered data, rejects missing catalog metadata, and passes existing per-slot/publication/historical-save checks.
- `UnrealCaptures/`: offscreen Vulkan captures from actual generated Gauntlet, Mama Turtle and Watering Hole plans, using original room/save assets and the port's atmosphere. These show the native station arrival animation. They were visually inspected. Logs confirm the actual area/room and zero emulated CPU opcodes.

Native/Game/Editor builds pass. This proves the paths listed above, **not exhaustive manual traversal of every modified tile or completion of the full VARIA option set**. The source ROM and upstream repositories remain pristine.

## Continue the full goal

Next: remaining layout/tweak C routines, then randomized world/door connections and their native transition handling. Capture the actual generated topology and bind validation/tracker to it; do not remove their vanilla-topology guards without that implementation. Red Brinstar Elevator/Golden Four area modes, mirror, Minimizer, ordered Scavenger, persistent general objectives, Tourian/escape/race, suits/Charge/Crystal Flash and remaining movement patches remain on the full plan. Complete dependent-control eligibility in the menu alongside those native behaviors.
