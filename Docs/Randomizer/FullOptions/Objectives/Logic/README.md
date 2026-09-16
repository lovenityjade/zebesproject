# Effective objectives in the solver and live tracker

This increment replaces the remaining four-boss assumptions in objective-aware validation and live queries. It does not yet enable nondefault goals in the public configurator: the original native objectives screen, notifications/map icons and remaining dependencies still need integration. The full thread goal remains active.

## Data and logic

`sm_objectives.configure_logic` restores the exact effective ordered goals and required count from the fingerprinted contract, without another random selection, expansion or nine-goal request clamp. It resets completion state and enemy totals, then binds percentage and regional goals to actual collected locations and the source-written membership masks. Upgrade completion uses the captured equipment masks. Historical manifests and relic hunts keep their existing fallback rules; their saved data is not migrated or rewritten.

`sm_validation` installs this configuration in the fresh bounded VARIA solver. `objectiveVerification` records each source objective step, inventory and collected checks at that point, extra AP paths, completed count and the first quota-reaching step. The historical item-only `progressionLog` retains its format. Mother Brain before the required quota is an error. These are logical routes, not recorded physical playthroughs. VARIA's exploration feasibility estimates use reachable locations and APs; they do not prove actual tile visitation.

Tracker metadata now describes the effective goals rather than always listing G4. Live queries use the same configuration and separately expose `completable`, `completed`, raw condition state, amount, target, percentage, hidden/revealed state and required/all-complete flags. Actual completion is never derived from having sufficient equipment. Callers without native evidence receive null progress fields.

## Native evidence and asynchronous consistency

`SmObjectiveSnapshot` is a 472-byte, versioned, read-only companion to the unchanged 2,688-byte tracker ABI 1. It carries the applied goal IDs, native evaluator values and persistent completion flags, including custom ZOE1 events. It does not change SRAM or seed-plan layouts.

Unreal reads both snapshots on the game thread, submits effective settings and native evidence to the embedded worker, and compares both again before publication. `sm_tracker_publish_objectives` independently rejects mismatched inventory/session/slot/fingerprint or changed objective progress. Rendering treats previously published colors/counts as pending when the objective evidence changes. Existing ABI 1 publication remains available to historical callers. The worker result retains objective JSON for the upcoming original-assets objectives UI; this increment adds no replacement popup.

## Validation scope

All runtime validation uses `/tmp/sm-native-generation-20260914` on gaming-pc. The source-generator override in `test-objective-logic-seeds.py` is fixture-only; production nondefault-goal guards remain in place.

- Seven generated pools cover G4, hidden two-of-three goals, item percentage, regional Chozo clear, animals, robots, beetoms, all upgrades, one-of-three quota, no objective and a reduced nonempty item pool. Fresh solver routes cover all 100 checks per seed. Boundary assertions distinguish counted checks from uncounted checks and received equipment.
- A separate effective eighteen-goal contract refuses quota completion at seventeen, accepts eighteen and resets cleanly to historical G4 afterward. This is a configuration boundary fixture, not an additional generated seed.
- The embedded oracle is checked against every item step and source objective-completion step in those routes. These progression inventories are fixtures, not seven native playthroughs.
- The native condition suite checks all 58 supported goal conditions, snapshot agreement/read-only behavior, real frame scheduling, statue gate operands, rollback and three-slot original SRAM persistence.
- A native-to-embedded-oracle fixture separates the red-fish interaction condition from its frame-latched completion, checks one-required versus all-complete, rejects stale ZOE1 evidence despite identical original event bytes, rejects wrong goal IDs/slot/session, and reloads the completion from original checksummed SRAM.
- Unreal profile fixtures include the seven general-goal plans alongside historical fixtures, exercising publication, roundtrip, malformed contracts and copy/clear. A Vulkan test uses the actual `FSMTracker` asynchronous request/publication path, then renders and navigates the native pause map with an area-randomized hidden-objective seed. It does not render the unfinished objectives screen.
- Compatibility checks pass four embedded default/relic generations and 52 historical native tracker pixel/state checks through the unchanged API.

Results and exact binary/module hashes accompany this document. Frozen Save Refill files and all three pinned upstream repositories remain unchanged. No local launch or local packaged-runtime replacement.

The inspected pause capture verifies the original area map and navigation. Its white pending markers follow the fixture's equipment/state changes after the startup oracle check; it is not a capture of a completed objectives screen or proof of fresh reachability colors at that later frame.

## Next work

Implement the original VARIA objectives pause screen, hidden reveal and progress presentation, notifications and objective/boss map icons. Finish the Bomb Torizo sleep and objective sound priority dependencies before enabling nondefault goals and testing their menu-to-generation path. Continue the original full scope: ordered Scavenger, modified/mirror worlds and Minimizer, Tourian/escape/race and remaining noncosmetic patches. This checkpoint does not establish full VARIA parity.
