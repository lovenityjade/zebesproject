# Fast elevators and split-specific HUD counts — 2026-09-15

This is another completed integration increment, **not completion of the full VARIA goal**. Alternate starts, world/door topology, mirror, Minimizer, Scavenger, general objectives, Tourian/escape/race and other gameplay/layout patches remain outstanding.

## Actual behavior

`elevators_speed` is accepted by generation and recorded as `fast-elevators-v1`. The native C port follows lioran's pinned `patches/common/src/elevators_speed.asm`: 3 pixels/frame in both directions, with downward departure limited to 24 transition frames. The timer uses the original patch's RAM alias at $0741, resets during gameplay, and is therefore part of native state rather than an external unsaved counter. The original 1.5-pixel routines remain in Vanilla and when disabled. Arrival keeps the original endpoint clamp and Samus alignment. Native capability bit 32 is checked before activating the plan.

The generator invokes VARIA's actual `RomPatcher.writeSplitLocs` on its in-memory FakeROM and reads only the resulting HUD location-ID lists. Those bytes never patch the game ROM. Each placement gets a fingerprinted `hudCounted` boolean; the `hud-counts-v1` contract is required and validated by Unreal. Full, Major, Chozo and FullWithHUD thus use upstream's restrictions, item class and location class predicates, including excluding Nothing. The port's subsequent tablet substitution includes tablets and excludes empty pickups.

Unreal translates these flags into native address order, persists them with the immutable seed and configures the HUD for the currently selected slot. Remaining counts subtract real native collection bits. Older manifests retain their historical all-check HUD; new manifests missing the required flags are rejected. The native API is also checked before publication/activation. Map tracker check squares continue to show all item locations: the split-dependent HUD count is a separate VARIA convention, not a reason to hide minor checks from the map.

## Proof

All execution tests used the isolated marked root on gaming-pc. No local game was restarted for this increment. The local user test launched earlier is separate. Upstream checkouts were not edited; the Save Refill release's recorded binary/ROM hashes were rechecked unchanged.

- `Seeds/`: five generated seeds, Full / Major / Chozo / FullWithHUD / FullWithHUD+tablets, all with fast elevators. The fresh solver verifies 100 checks and the appropriate completion route. Their counted totals are respectively 80, 34, 25, 16 and 36.
- `native-verification.json`: actual compiled elevator routine calls in an initialized native game, in Vanilla and randomized modes, off/on/combined flags, both directions, 24-frame cap/reset, fractional coordinates and four endpoint distances. HUD checks cover every populated graph region for each seed, half-collected fixtures, switching to historical counting and back. These are controlled native routines and RAM fixtures, not a manual traversal of every elevator.
- `runtime-verification.json`: native plans apply, restore Vanilla and reapply; 25 embedded live-oracle queries replay progression inventories and verify accessibility difficulty/technique constraints.
- `profile.log`: new native flag mapping, counted-contract parsing, missing-field rejection and historical plans, plus existing independent-slot/publication/immutability regressions.
- Native, Unreal Game and Unreal Editor builds passed. `build-hashes.json` records the future-launch binaries. The Save Refill release was not rebuilt or restaged.

Reproduce using `Scripts/test-elevators-hud-seeds.py ROOT`, `Scripts/test-elevators-hud.py ROOT`, `Scripts/test-seed-options-runtime.py ROOT elevators-hud`, and the existing Unreal profile selftest. `Scripts/test-hud-counts-render.py ROOT` captures the actual native HUD in gameplay; this capture is the native pixel renderer, not an Unreal atmosphere capture.
