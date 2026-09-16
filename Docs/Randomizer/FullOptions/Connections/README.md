# Native boss connections — 2026-09-15

Boss connections are integrated into the existing English World & escape settings and native Generate Game workflow. A/B/C store independent reciprocal mappings. Generation captures VARIA's actual door-writer inputs; the solver, live tracker and C runtime consume the same topology. Boss entry/exit doors, area flags, compatible/incompatible arrival geometry, original door ASM, invincibility, Kraid background transfers, Phantoon/Draygon BG2 cleanup, music and CRE reload data have native counterparts. No injected 65816 routines run.

The four boss dependencies are accounted for: `door_transition.ips` is C behavior, while `Phantoon_Eye_Door`, `WS_Main_Open_Grey` and `WS_Save_Active` are four reviewed ROM-data bytes in three appended world IDs. `WS_Save_Blinking_Door` is inserted only in room `$CAF6`, state `$CB08`; insertion preserves the RAM descriptor scratch and avoids duplicates. Shinespark cancellation uses a private one-shot flag rather than conflicting with the pause menu's `$0741` storage.

The eight access-point IDs and 32 allowed directed combinations are compiled from the pinned source by `Scripts/build-boss-connections.py`. `audit.json` includes original source descriptors, the boss patch set and state-specific PLM. `Randomizer/connection_catalog_history/boss-v1.json` freezes these semantics. The world catalog now has 46 IDs, 89 data spans and 65,805 backup bytes. Previous world IDs remain identical; `world_catalog_history/boss-connections-v1.json` archives the current catalog. The generator asserts that boss restore domains do not overlap ordered world patches.

Unreal validates reciprocal destinations, endpoint roles, catalog fingerprints, graph pairs and tracker/world agreement. A missing native capability fails before Generate Game. Configuration precedes native generation commit; applied routes only change when the item/world transaction succeeds. Selecting Vanilla restores original doors, music and room data. Old plans with no topology retain vanilla connections.

## Evidence

All runtime work below ran in `/tmp/sm-native-generation-20260914` on gaming-pc. No local game launch or user-save modification occurred.

- `Proofs/topology-verification.json`: all 24 boss permutations, 48 embedded-oracle queries producing seven distinct inventory/accessibility combinations, eight malformed-topology rejections, two new vanilla seeds and four boss seeds. Seeds 15092402–15092405 pass fresh 100-check solver/completion verification with distinct mappings, optional layout settings and alternating No Advanced Techs.
- `Proofs/tracker/verification.json`: all 400 next checks in those four solver progression logs are reachable according to the live embedded tracker; collected checks disappear. Inventory and boss flags come from the solver log. This is not physical traversal or combat proof.
- `Proofs/native/verification.json`: 24 permutations/192 native door-setup cases, full-ROM expected bytes, invalid-plan rollback, Vanilla restoration, exact scroll/VRAM/iframes/velocity/shinespark effects, and four state-specific save-door cases. Zero emulated CPU opcodes.
- `Proofs/rooms/verification.json`: all 32 directed arrivals load actual native room geometry and run 120 frames; bounded positions, health and area are checked. These earlier controlled loads use the debug destination fixture and full equipment, not walking through each source door. Two representative incompatible-arrival PNGs were inspected.
- `Proofs/world-transactions.json`: 50 full-ROM data-layer transaction/restore cases with the 46-ID catalog.
- `Proofs/bank-1/verification.json`, `bank-2/verification.json`: all four boss plans through native new-file menus, real initial SRAM, reload, later save, copy/clear, failed publication rollback and independent Vanilla. Door bytes are checked against each slot's mapping.
- `Proofs/historical-bank.json`: old start plans still pass the native save-bank regression with original boss connections.
- `Proofs/connections-profile.log`: 25 Unreal profile fixtures pass publication, reload and malformed-plan rejection. The fixture request now carries No Advanced Techs explicitly; the first failed test correctly rejected a fixture which omitted it.
- `Proofs/connections-render.log` and `boss-unreal.png`: actual offscreen Vulkan render of seed 15092402, source 3 (Kraid outside) to destination 4 (Phantoon arena), room `$CD13`, zero emulated CPU opcodes. Original room graphics, Samus and boss flame effects are visible. This is a controlled arrival, not an entire seed playthrough.

Native, Unreal Game and Unreal Editor builds passed. `Proofs/build-hashes.json` records future-launch staging and verifies the frozen Save Refill release and pristine upstream checkouts.

## Remaining full-goal work

This does not complete full VARIA parity. Native area/door-color randomization, mirror, Minimizer and its extra saves/doors, ordered Scavenger, general objectives, Tourian/escape/race, remaining gameplay patches, and their tracker/counter/persistence/rendered tests remain required. VARIA portal-map symbols and their exploration semantics also need integration with the native map presentation; current check availability already uses the actual boss graph. Save-station travel stays paused and the Speed Booster visual request stays queued.
