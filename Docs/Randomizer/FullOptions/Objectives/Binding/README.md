# Objective contracts through generation, Unreal and A/B/C

The subsequent [Logic checkpoint](../Logic/README.md) connects effective general-goal solver/tracker semantics and native progress evidence. The UI/dependency work below remains outstanding.

This increment binds the evaluator from [Conditions](../Conditions/README.md) into newly generated ordinary-Tourian seeds. Nondefault objective options remain generation-gated: shared general-goal solver/tracker semantics, original goal UI/notifications/icons and remaining dependencies are still unfinished. It is not full VARIA objective parity.

## Effective data and native activation

`sm_integration.py` now calls `sm_objectives.capture` after VARIA has selected the effective pool, split and world. The actual source data writers supply expanded/selected goal identities, required count, flags, item/area membership, equipment masks and world totals. The post-writer goal names/count replace the earlier summary. The complete contract is part of the seed fingerprint and is copied into tracker settings. New standard seeds declare `native-objectives-v1`; relic hunts retain their separate quota/ship completion and have no ordinary objective contract. Historical manifests are not rewritten.

`FSMSeedPlan` stores a value-owned `SmObjectivePlan`. `ReadPlan` checks the compiled shared catalog, source goal identities, quota/masks, exact full-layout totals, region membership against the HUD, world mode and effective summary. It requires the tracker to carry the identical contract. The generated `SMNativeObjectives.inl` comes from the same pinned catalog compiler as native C.

`FSMSystemMenu` loads the objective API, configures it after area routing, and includes it in generation commit and each slot's load/copy/clear path. A Vanilla, relic or historical slot clears the pending objective configuration. Existing transaction machinery persists the fingerprinted plan alongside native SRAM; the C evaluator changes only when its seed/world transaction succeeds. The profiles capability list recognizes the new version and retains old contracts.

The public frame entry previously called `sm_seed_frame` both before and after `RtlRunFrame`. The earlier condition test only proved one goal per helper call, so placing evaluation there advanced twice per real frame. Evaluation now runs once after the native frame. The new test exercises `sm_step` itself and proves successive counts of 1, 2 and 3 across three frames.

## Verification

All runtime checks ran in `/tmp/sm-native-generation-20260914` on gaming-pc. Native, Unreal Game and Editor builds passed. Proofs are tied to hashes in `build-hashes.json`.

- Four new embedded generations (`15092800..15092803`) cover Full, FullWithHUD with area routing/Golden Four start, Chozo with boss/door randomization, and relic hunt. Each passes a fresh complete 100-check solver progression. Standard manifests have the effective four G4 objectives and matching native/HUD/tracker membership; relic completion remains independent. This validates current supported goals, not enabled arbitrary-goal seeds.
- The live embedded oracle agrees with all 400 item-check steps from those progressions, including prior collected checks and each effective topology. Inventories/bosses are progression fixtures, not four physical playthroughs.
- Native menu tests cover three mixed banks: Full/area, Full/Chozo, and new/historical, with Vanilla A. Generation/save failure rollback, initial autosave, reload, later saves, copy and clear pass. New objective seeds independently acquire one boss goal, save it through original checksummed SRAM, and retain it after a complete reboot/load. Another fresh seed starts with zero completed goals. These are controlled boss-bit fixtures; boss fights were not simulated.
- The 58-condition native suite passes against the new frame hook, including its new real-frame scheduling check, quota/SFX, invalid data, SRAM and pending-world checks.
- Unreal profile self-tests pass all 38 fixtures. New plans survive publication/reload and copy/clear; malformed goal catalogs, names, quota, HUD membership, missing contracts and tracker disagreement are rejected. Old plans still load without an objective descriptor.
- `test-objective-binding-unreal.py` parses the new seed `15092801`, applies its four/required-four objective contract in the running C core, traverses an area fixture, renders the original pause map, navigates and resumes under Vulkan. The screenshot was inspected. It does not show an objectives screen; that screen remains to implement.

Only the isolated gaming-pc runtime was staged. No local game was launched or local packaged runtime replaced. `preservation.json` verifies the frozen Save Refill files and pristine native-core/disassembly/VARIA source revisions.

## Next work under the full goal

Replace the vanilla-goal assumptions in `sm_validation.py` and `sm_live_tracker.py` using the effective contract. Their general-goal behavior must include actual regional item membership, upgrade masks, required-vs-total goals and progression evidence. Preserve known completion separately from logical ability to complete an interaction. The current tracker snapshot contains original events/bosses but not the ZOE1 custom objective bitmap; general goal progress must not be inferred from inventory alone.

Then implement original VARIA objective pause assets, progress notifications and objective/boss map icons, plus Bomb Torizo sleep and completion-sound priority dependencies, before enabling and validating nondefault goals. Continue ordered Scavenger, modified layouts/Minimizer, Tourian/escape/race and all remaining non-cosmetic patches. The full thread objective remains active.
