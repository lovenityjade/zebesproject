# Native door indicators and complete layout group — 2026-09-15

VARIA's door indicators now run as original-asset PLMs in the native game. The full `layoutPatches` group can be generated, including its individually selected patches, together with VARIA Tweaks. This removes the remaining indicator guard; it does **not** complete area/boss/door randomization, mirror, Minimizer or the rest of the full VARIA objective.

## Data and native integration

`Scripts/audit-door-indicators.py` reconstructs `door_indicators_plms.ips` byte for byte: 784 bytes and 16 PLM definitions. Every instruction uses an existing native C handler. This blob is PLM data, not new 65816 machine code. The new world catalog appends ID 42 and includes all 18 immutable location templates, with no changes to the previous 42 IDs. The catalog builder preserves every archived catalog's ID ordering and rejects room-data overlaps with the separately restored indicator entries.

Generation reads `DoorsManager.getIndicatorPLMs` with the actual Standard/AreaRando/DoorRando flags. It records the selected location IDs and actual indicator PLM colors in `nativeContext.world.indicators`, requires `native-door-indicators-v1`, and binds them to the seed fingerprint. The current unshuffled world has 13 standard indicators. Native definitions cover all 18 templates, but area/door-randomized generation remains gated elsewhere. Beam-colored indicators still require the separate beam-door implementation and fail explicitly.

The extended `sm_start_configure_world` API validates the full slot configuration before replacing it, including location uniqueness, supported PLMs and correct orientation. A/B/C and the legacy fixture slot hold independent selections. The applied selection changes only when the world/item transaction succeeds. Older `sm_start_configure` callers retain the empty-indicator behavior. Catalog compatibility preserves historical seeds.

Two indicator locations replace six-byte room entries; the remaining locations add the corresponding PLM during both native room-loading paths. Original replacement entries restore on Vanilla selection. Added PLMs retain the opposite door's real opened bit and are installed without duplicates. The original native closing-cap lookup recognizes their six-byte headers and uses their closing lists. Shot response borrows the blue-door opening scripts and does not mark the opposite colored door opened. Once that opposite door bit is set, the PLM installs the original blue-door BTS/tiles and deletes itself.

The native X-ray scan now excludes these high-address indicator PLMs from the old item-PLM range check. Otherwise it would interpret them as item definitions.

Unreal parses and persists the indicators with the full seed, rejects malformed/missing metadata and missing capability tags, configures each slot through the new API, and passes the configuration before committing native generation. This applies to normal gameplay, copy/reload and the isolated visual fixture.

## Verification and limits

Tests run on gaming-pc in the marked isolated root, with no local launch or changes to user saves.

- `Proofs/native.json`: all 18 location templates, in-place replacements and Vanilla restoration, idempotent room insertion, malformed/unsupported/orientation rejection, pending-versus-committed slot isolation, and all 16 color/orientation PLMs through the actual native PLM handler. Each follows exactly nine blue frames, nine colored frames and repeats. Shot response, opposite-door-bit preservation, permanent-blue/delete behavior, X-ray safety and native SRAM opened-bit save/reload pass. These are controlled routine/RAM fixtures, not manual traversal of every room.
- `Proofs/Seeds/` and `seeds.json`: four real seeds (layout off, indicator only, all layout patches, all layout plus all VARIA Tweaks). Each has a fresh 100-check solver/completion proof and matching spoiler/progression items.
- `Proofs/tracker.json`: 20 live in-process oracle queries using the effective settings, native seed selection and Vanilla/reapply checks. Green states respect the seed's technique and difficulty limits.
- `Proofs/slots.json`: native generation menus, independent B/C configurations, real initial autosave/reload, failure rollback, copy/clear and later-save behavior. `historical-slots.json` repeats the native bank workflow with historical Gauntlet/Mama Turtle plans.
- `Proofs/profile.log`: 21 Unreal fixture plans (13 prior starts, four prior tweaks, four new layout seeds), publication/reload and malformed-data rejection. New indicator lists retain their exact per-slot values.
- `Proofs/world.json`: 47 full-image data transaction/restoration cases, including ordered overlap handling and code-only selections. Indicator room-entry writes are tested separately in `native.json`.
- `Proofs/contract.json`: current and all archived catalog hashes accepted, malformed contracts rejected, and still-unimplemented door/world randomization rejected.
- `Proofs/Unreal-indicators-on.png` and `Unreal-indicators-off.png`: actual offscreen Vulkan renders of the same original room `$948C`, with the green indicator phase versus the original blue door. Both captures were visually inspected. Logs confirm the room, 13 versus zero indicators and zero emulated CPU opcodes. This is a controlled room transition from a fresh generated save, not a manual walk there.

Native/Game/Editor builds pass. Artifacts and Python/catalog files are atomically staged for future local/package launches; the frozen Save Refill release remains unchanged. `Proofs/build-hashes.json` identifies the staged artifacts. No local runtime was started.

## Remaining full objective

Continue with randomized area/boss/door connections and native transition/collision handling, binding the actual topology to the solver and live tracker. The two area-only starts, mirror/Minimizer, Scavenger order, general objectives, Tourian/escape/race and remaining suit/Charge/Crystal Flash/movement patches remain required. Do not mark the full integration complete based on the layout group.
