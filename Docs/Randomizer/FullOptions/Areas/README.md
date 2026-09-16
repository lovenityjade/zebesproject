# Native area routing — implementation checkpoint

This increment now connects native area routing to generation, per-slot plans and the tracker. Area Full and Light are enabled in the development candidate; portal-map presentation remains unfinished. The full option-parity goal remains active. The future-launch runtime and immutable Save Refill release have not been replaced by this candidate.

## Source contract

`Scripts/build-area-connections.py` compiles the pinned VARIA source at `72ec1f30b700442d0c30aad6c43801bd64f1e2a5`. `Randomizer/native_areas.json` and `Native/sm_areas.inc` contain a distinct 32-AP domain; the published eight-AP boss catalog is unchanged. Each of the 1,024 directed combinations is taken from `GraphUtils.getDoorConnections`, including reciprocal self-links needed by later world modes. Descriptor coverage does not mean every combination is legal for an ordinary generated area seed: the generator's graph constraints still decide that.

The native module restores its owned bytes before every committed world change. It handles destination room/door geometry, original scroll routines, incompatible-arrival velocity/pose reset, 128 invincibility frames, one-shot shinespark cancellation, Crocomire's CRE/BG2 exit corrections, room music fields and West Ocean's incoming-door sky selector. Boss and area arrivals share the existing native counterpart of `door_transition.ips`; no custom 65816 routine is executed.

The 6,509 dependency bytes include the two Maridia save records and map icons, Crab Shaft and Below Botwoon compressed level edits, the West Sand Hall door, the additional Below Botwoon door list, Wrecked Ship activation and blinking-door replacements. The 31 CPU bytes of `area_rando_doors`' full refill routine are excluded; `$8FF701` dispatches to C and restores energy, reserves and all ammunition at the Tourian elevator. The entire `door_transition.ips` is excluded from the data image and retained in the audit.

`sm_areas_room` inserts the 27 source PLM templates, honoring the Wrecked Ship state filter, preserving the RAM descriptor scratch area, and avoiding duplicates when boss routing inserts the same save-room door. Existing world/start IDs stay unchanged.

## Patch precedence

Source `getPatchSetsFromPatcherSettings` applies the area group before layout/tweaks. `getStartDoors` adds blinking-door replacements after those groups, followed by indicators/start patches. The native candidate restores all owned data first, applies area base data before the ordered world layer, then applies blinking replacements and routing. The reviewed cross-layer overlaps are:

| Area patch | Existing world patch | PC address | Result |
| --- | --- | --- | --- |
| WS_Main_Open_Grey | WS_Main_Open_Grey | 10BE92 | Identical byte |
| WS_Save_Active | WS_Save_Active | 7CEB0 | Identical byte |
| Blinking[Le Coude Right] | moat.ips | 1085DD | Blinking replacement wins |

This audit must be repeated when either catalog grows; it is not a promise that future patches commute.

Nine blinking replacements also overlap existing door-color identities: GreenPiratesShaftBottomRight, KihunterBottom, GreenHillZoneTopRight, NoobBridgeRight, KronicBoostBottomLeft, CrocomireSpeedwayBottom, CrabShaftRight, LeCoudeBottom and RedBrinstarElevatorTop. The combined generator plan must preserve VARIA's forced-open area-door choices; these cannot be independently assigned a random beam requirement. The native color layer runs after the blinking layer and leaves color-zero entries untouched.

## Validation

Native compilation passes in `.tmp/areas-native`. Testing is isolated under `/tmp/sm-native-generation-20260914/areas` on gaming-pc. The candidate has not been staged to the user's game.

`Scripts/test-native-area-connections.py` checks 32 reciprocal mappings covering all 1,024 pairs, original scroll effects, arrival state, per-slot selectors, invalid/asymmetric/out-of-range tables, rejected item-transaction rollback, full-ROM restoration, source-defined patch precedence, all added PLMs, shared boss/area PLM deduplication, and the Tourian refill. These are native controlled fixtures, not generated seed playthroughs.

`Scripts/test-native-area-rooms.py` loads each of the 32 destinations from the four source directions through the real room loader, then runs 440 gameplay frames. This includes graphics/level decompression and Samus's save-load arrival fanfare. An initial 20-frame wait was insufficient: synthetic reloads accumulated delayed fanfare music and produced unsupported SPC commands at Crab Shaft. The corrected fixture waits for the arrival sequence and asserts an empty music queue. All 128 corrected cases pass without that audio warning. No audio-engine workaround was introduced.

`Scripts/test-native-area-transitions.py` prepares a source room, finds its actual door block in the loaded level/BTS/door list, and invokes the original horizontal or vertical collision routine. The game then runs the real fade/scroll/load transition to the configured destination, without destination-room injection during the transition. Twelve cases pass, including Crocomire → West Ocean, West Ocean → Crab Shaft, incompatible directions and elevator endpoints. These controlled collisions are stronger than destination-loader fixtures but are not a manual or generated-seed playthrough.

`Scripts/test-area-regressions.py` runs the existing boss-routing and world-data suites against the area candidate, with separate output directories. All 24 boss permutations and 50 world transactions pass.

Foundation candidate SHA-256: `52b1e958ea90f355c222a78dde7b0d3e638d5d52a06ec2723270d1b81479578a`. The local candidate and the isolated remote library match. `Proofs/build-hashes.json` records source/test hashes. `Proofs/native.json`, `rooms.json`, `transitions.json`, `boss-regression.json` and `world-regression.json` record the final results. Native tests report zero emulated CPU opcodes and unchanged source ROM. Final logs contain no unsupported SPC warning or assertion failures. Representative native PNGs were inspected, including the real Crocomire → West Ocean transition; they are not Unreal-render evidence.

The existing future-launch Game/native package is still the DoorColors increment. Save Refill's three manifest hashes and the pristine native-core/VARIA checkouts were verified unchanged. Diagnostic audio instrumentation was built only under `.tmp/area-audio-diagnostic` and the remote `areas/audio-diagnostic.so`; it is not the tested final candidate or a distributed runtime.

## Integration checklist (foundation checkpoint; superseded below)

1. Carry effective area mappings through Python capture/validation, native plan hashes, Unreal parsing, independent A/B/C configuration/activation, generation commit, copy/clear/reload and rollback.
2. Capture the complete upstream initial-door plan (`getStartDoors`, `getBlueDoors`, start-specific doors) with a new versioned contract. Historical seeds must keep their previous semantics. New non-color seeds must receive the fixed save/refill-door bits too.
3. Connect the two area-only starts, generation dependencies and effective solver/tracker roots; validate actual area and area+boss+door seeds, including all progression steps.
4. Implement native VARIA map portal/door symbols and artificial exploration of relocated portal tiles. `coversTile` requires the matching graph-area tile counter; these behaviors are not claimed by the current arrival hook.
5. Test real transitions and saves in generated seeds, and inspect Unreal Vulkan renders. Destination-loader fixtures alone do not prove physical traversal, tracker integration or persistence.

Then continue the original Minimizer/mirror/ordered Scavenger/general-objectives/Tourian/escape/race/remaining-patch scope. This checkpoint does not reduce that objective.

## Generation, initial doors and slot integration — 2026-09-15

The development candidate carries a separate `nativeAreas` contract through the Python generator, fresh solver, live tracker, Unreal plan reader and A/B/C slot activation. The boss contract retains its original eight-endpoint namespace. Ordinary area plans require all 32 unique endpoints in reciprocal pairs; unexpected endpoints, descriptor drift and mismatched graph/native targets are rejected. Area Full, Area Light, area plus boss plus door colors, Golden Four and Red Brinstar Elevator now generate actual validated plans.

New plans also require `native-initial-doors-v1`. The complete door-bit list is derived from the actual upstream `getStartDoors`, `getBlueDoors` and start AP data. Native save initialization applies the fixed save/refill bits and start-specific doors; historical plans without the new contract retain their old behavior. Lists must match their start exactly. Invalid replacements preserve the accepted configuration; mismatched starts and area-only starts without an area world are rejected. Configuring and clearing slots updates area routing before the dependent start contract.

`Scripts/test-native-initial-doors.py` checks all 15 starts against the pinned upstream definitions, all four selectors, malformed/repeated/missing door bits, spawn mismatch, the area dependency guard and legacy empty lists. These are real native initializer calls with controlled RAM, not persistence claims. The independent slot suite covers generation commit, autosave, reload, later-station saves, rollback, copy and clear.

Six generated seeds pass the fresh solver's 100-check completion verification, and `Scripts/test-area-tracker-progression.py` passes all 600 progression steps through the embedded oracle. The latter feeds each logged pre-pickup inventory and the actual initial-door bits; it checks accessible next checks and cleared prior checks. It is logic evidence, not a physical playthrough. Three native banks cover all six plans, including simultaneous area/boss/color configuration and both area-only starts. A separate historical bank also passes with the new library. Game and Editor builds pass.

`Scripts/test-generated-area-transitions.py` passes 15 real collision/fade/scroll/load transitions using five actual generated worlds, with prepared source rooms and full-equipment fixtures. It also confirms the nine overlapping area-door identities remain blue in the combined area/boss/color seed. No destination injection occurs during the transition. This is not a manual seed playthrough or proof of traversing these routes with only the starting inventory.

The 35-fixture Unreal profile suite passes generation publication, reload and malformed-data rejection. Three real offscreen Vulkan captures were inspected: Golden Four (room A5ED), Red Brinstar Elevator (A322), and a combined-seed arrival through source 31 to Glass Tunnel Top (CEFB). The last capture uses the native destination-loader fixture, while the separate native suite proves collision-driven transitions. All three logs report zero CPU opcodes; no unsupported SPC warnings or assertion failures occurred in these suites.

Results and source/binary hashes are in `Integration/`, separately from the foundation's earlier `Proofs/`. Integrated candidate SHA-256: `28e8e715243eb4cd78664787ccd77a93a169c9f3b567b09e98aafe8644fa4ebb`. The six generation records retain the earlier foundation library hash because they ran before the integrated native rebuild; their Python generator source is the integrated source. Native save/transition and Unreal proofs use the new matching library. This distinction is recorded in `Integration/build-hashes.json`.

Only the isolated gaming-pc runtime was updated for these checks. Local `Native/build` and the packaged future-launch runtime remain at DoorColors while area map integration continues. The Save Refill manifest's three files and all three pinned source repositories were reverified unchanged. No local game was launched.

Remaining area work: native VARIA portal/door-map assets and symbols, artificial exploration of relocated portal tiles, and the matching graph-area tile counters for `coversTile`. The initial routing and save tests do not prove these behaviors. Continue mirror/Minimizer, ordered Scavenger, general goals, Tourian/escape/race and remaining gameplay patches afterward; the full goal is not complete.

### Map sprite follow-up

[MapIcons](../MapIcons/README.md) now supplies the original portal/door sprites on native map and minimap, with source visibility rules and committed routing. The eight relocated display positions still need real-door exploration hooks and correct graph-area counters; the full area/map integration remains incomplete. The separately recorded explored-map render is an explicit fixture, not evidence of natural discovery.
