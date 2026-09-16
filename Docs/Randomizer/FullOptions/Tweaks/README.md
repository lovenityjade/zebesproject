# Native VARIA gameplay tweaks — 2026-09-15

The complete `variaTweaks` group now has native implementations: Wrecked Ship Energy Tank access (existing reviewed room data), the Lower Norfair Chozo Space Jump bypass, and pickup-triggered Bomb Torizo awakening. The English editor already exposes the group and its individual selections. Generation, immutable A/B/C plans, effective logic and live trackers share the selected patches. **Full VARIA integration remains incomplete.**

## Behavior and source fidelity

`bomb_torizo.ips` changes the grey door instruction and statue pre-instruction to check the original item's PLM header at `$1C83` (array index 38). Picking up any item there deletes that PLM, waking the statue without requiring Bombs. Entering with Bombs while that item is still present no longer wakes it. The native overlay changes the same two routines and preserves their original branch/timer behavior. VARIA's special objective-controlled permanent sleep policy is not enabled; the corresponding nonstandard objective modes remain blocked.

`LN_Chozo_SpaceJump_Check_Disable` removes only the Space Jump requirement. The native routine still requires downward collision and one of the original morphed poses. It retains the original event, enemy activation, block clearing, movement lock and hand PLM spawn. The associated `ln_chozo_platform.ips` room data is selected alongside it by VARIA's actual patch set.

The 42-entry world catalog retains the first 40 data IDs and adds two code-only IDs. Their original instruction spans are recorded as `sourceSpans` for auditing, but their bytes never enter the native ROM image. The C counterparts use the **committed** world selection; configuring another slot or rejecting an invalid item plan cannot change the currently applied behavior. Vanilla and historical seeds retain their original rules.

The archived catalogs `vanilla-starts-v1.json` and `vanilla-tweaks-v1.json` bind existing IDs and source definitions. The builder rejects silent changes to archived entries, and the runtime accepts compatible historical hashes. No user save or frozen Save Refill release was modified.

## Evidence

All runtime tests ran in the marked isolated directory on gaming-pc.

- `native-verification.json`: 84 controlled cases invoking the actual C PLM routines after a native game boot. These cover all combinations of the Bombs bit/item presence, valid and invalid Chozo poses/collision directions, independent A/B/C selections, Vanilla fallback, native pickup-PLM deletion, and rejected/uncommitted selection isolation. This is routine-level evidence, not a manual room playthrough.
- `Seeds/` and `seeds-verification.json`: four actual generated seeds (off, Bomb Torizo only, Lower Norfair Chozo only, all three tweaks). Each has a fresh 100-check solver proof and completion route, spoiler/progression agreement, and matching selected native patches and tracker logic patches.
- `tracker-verification.json`: 20 live in-process queries across those seeds' progression inventories. Green states obey the effective skill/difficulty limits; native item plans restore Vanilla and reapply successfully.
- `slots-verification.json`: original native menu generation, initial autosave, B/C reload with different tweak selections, failed publication rollback, copy/clear and later-station saves. These exercise native bank behavior, not Unreal profile serialization.
- `profile.log`: Unreal independently parses, publishes and reloads 17 plans (13 historical start fixtures plus four new tweak fixtures), including their ordered native selections; malformed metadata and existing stale-publication regressions pass.
- `world-verification.json`: 46 full-image transaction cases, including each patch, combinations, reverse order, duplicate rejection and Vanilla restoration. Code-only IDs do not write SNES instruction bytes. Source ROM unchanged; zero emulated CPU opcodes.
- `contract-verification.json`: both historical catalog hashes accepted; malformed IDs, custom patch names and still-unimplemented modes rejected.

Native, Unreal Game and Unreal Editor builds pass. The latest native/Game artifacts and Python integration are staged for future local/package launches without launching the local game. `build-hashes.json` records exact artifacts and frozen-release preservation.

## Continue the full objective

Door indicators are still generation-gated pending native room insertion/replacement and lifecycle/rendering proof. The new [exact data audit](../DoorIndicators/README.md) corrects their earlier classification as executable machine code. Complete those room operations to enable the full layout group, then continue randomized world/door topology, area-only starts, mirror/Minimizer, ordered Scavenger, general objectives, Tourian/escape/race and remaining combat/movement patches. Existing guards must stay until the corresponding native behavior and solver/tracker context are implemented.
