# Implementation checkpoint — 2026-09-15

## Current scope — user revision, 2026-09-16

The user explicitly removed **Mirror and Race** from the requested integration.
Stop both workstreams. These modes are excluded from completion requirements;
earlier chronological checkpoints listing them as required are superseded.
The English settings editor hides their controls. The generator rejects
non-neutral `logic`/`raceMode` values while retaining neutral legacy fields.
No existing save or generated plan is rewritten. Mirror's guarded prototype
and proofs remain archived in the source tree, not a supported game mode.
All remaining randomizer features and preservation requirements remain in scope.
Animals integration is the latest completed feature block. Final verification
of the revised overall scope is still required before marking the goal complete.


## Current increment

Candidate **E, 16x16** replaces the earlier A direction. It is a technical import of the user's reference sheet, with binary alpha and a limited palette; the failed imagegen drafts are not runtime assets.

Chozo Relic Hunt now has native pickups, a separate collection kind, per-slot quota, real item-bit persistence, an item-tracker icon/count, a pause-map quota line, and context-appropriate VARIA HUD notifications. Hidden and Chozo carrier PLMs retain their original reveal scripts. Relics never grant a Reserve Tank or equipment. Separate Unreal turquoise light/spark events accompany visible and collected relics. Completion uses the original ship re-entry/takeoff and real credits pipeline, bypassing the Mother Brain/Zebes explosion cinematic. Credits include the fragment count.

The English randomizer menu exposes No Advanced Techs and the hunt's placed/required counts. Generation remains in the native new-file menu. Ready seeds are immutable; A/B/C have independent requests and quotas, including copy/reload. Publication re-reads the current slot request and rejects a changed quota or slot identity.

No Advanced Techs explicitly sets every upstream technique flag. It retains ordinary WallJump, ShineSpark, MidAirMorph, CrouchJump and UnequipItem, disables other techniques, and disallows heat runs. It is not a promise of easy combat. Historical seeds are unchanged; regenerating is required to change logic.

Fragments replace surplus ammo/empty placements while preserving a conservative ammo floor. The final pool is solved again. Every accepted seed must retain all 100 reachable checks; the progression log also verifies returning to Landing Site with the inventory at the quota step. The pre-replacement generator log is labelled separately from the final progression log. This conservative pool validation can reject configurations which might otherwise be finishable without collecting everything.

## Work still outstanding from the broader request

The full non-cosmetic VARIA catalog is audited and typed in `sm_options.py`, but **full native integration is not complete**. The new nine-page editor exposes the audited non-cosmetic form; see the MenuEditor checkpoint below. Mirror/Minimizer and other modified worlds, their goal/count dependencies, alternate Tourian/escape/race and remaining gameplay patches still need implementation and end-to-end validation. General objectives for full original/area-randomized layouts with ordinary Tourian are now enabled in the development candidate; see Objectives/MapDependencies below. Area topology, all 15 starts, boss routing and randomized door requirements are connected in the development candidate; see their later checkpoints below. Changed settings whose native behavior is missing fail explicitly; passing arbitrary ROM patches through the C port would not implement their code. Do not advertise this increment as complete VARIA option parity.

Save-station travel remains disabled/paused at `../SAVE-STATION-TRAVEL-WIP.md`. The Speed Booster visual note is still queued. The user's active local game and SRAM are not restarted or overwritten during this work.

## Evidence and limits

`Proofs/seed-verification.json`: three 30/20 hunt seeds and a No Advanced Techs baseline, all 100 checks verified, plus rejection of unsupported area topology and an invalid quota. Solver proof is not a full manual playthrough.

`Proofs/ending-result.json`: real native pickup grants, refusal to depart at 19/20, native ship save/reload retaining 19, actual takeoff/credits at 20, no Mother Brain or escape event, zero emulated CPU opcodes.

Native room/counter captures and Unreal rendered captures are stored separately. The visual fixture intentionally substitutes a relic at the Morph pedestal for controlled observation; it is not claimed to be that generated seed's spoiler placement.

Final build and staging: native + Unreal Game + Unreal Editor builds passed. `Proofs/build-hashes.json` records the exact artifacts. The library was atomically replaced for future launches, with the previous binary retained in `.tmp/full-options-baseline/libsm_native-before-relic.so`. The local packaged runtime was staged too; the running local game was not restarted.

Final offscreen Vulkan render on gaming-pc: `SM_RELIC_VISUAL_TEST PASS collected=1 bursts=1 glowFrames=230 cpu=0`. These events pass through the actual Unreal particle/light presentation. See `relic-unreal-idle.png` and `relic-unreal-pickup.png`. `profile.log` contains `SM_RELIC_PROFILE_TEST PASS` and the historical profile regression pass.


## Chozo Tablet pickup message — 2026-09-15

A tablet now opens the original native item-message animation with `CHOZO TABLET` and `X COLLECTED OUT OF Y`. The collected value includes the current pickup; the denominator comes from that slot's seed quota, not a fixed 15. The native uppercase font, borders, PPU restore and acknowledgment timing are reused. A temporary Reserve message carrier is intercepted only for tablets; normal Reserve Tanks retain their original text and grant. No inventory equipment is awarded for a tablet. Pickup sparks/light are deferred until the dialog closes so they survive the gameplay freeze.

Completion remains quota then return to Samus's ship: collecting the last required tablet does not instantly interrupt the room with credits. The ship saves the run and launches the real credits without requiring Mother Brain. This is the behavior explained to the user in this increment.

`TabletMessage/verification.json` and the native PNGs verify a real room collision at 1/15, a normal Reserve Tank afterward, and injected native PLM grants at 15/15 and 16/20. Later counts use a controlled collected-bit fixture; this is not a manual traversal claim. Source ROM remains unchanged; emulated CPU opcode count is zero.

The updated native ending regression also passed with the pickup dialogs in place: refusal at 19/20, save/reload at 19, then ship departure and real credits at 20 (`TabletMessage/ending-result.json`). Native, Unreal Game and Unreal Editor builds passed. Future-launch native and packaged game binaries were atomically staged; the running local game was preserved. `TabletMessage/build-hashes.json` records this increment separately from the earlier proofs.

Updated offscreen Unreal Vulkan capture on gaming-pc passed: `SM_RELIC_VISUAL_TEST PASS collected=1 bursts=1 glowFrames=230 cpu=0`. `TabletMessage/relic-unreal-message.png` shows the actual native dialog within the enhanced widescreen world; `relic-unreal-pickup.png` captures gameplay after dismissal. Both were visually inspected.

## User correction: complete seed configurator

Live site and dynamic editors audited on 2026-09-15: [reference study and implementation contract](../../References/SMRandomizerUI/Live-2026-09-15/README.md). The full menu was requested and remains undelivered. At the time of that audit, the Unreal request omitted `options`, `techniques` and `skillSettings`; fixed strings and unavailable rows did not satisfy the request. Complete per-slot UI, serialization, native behavior and tracker parity are required together. This study made no game-code changes.


## Save Refill release and full seed editor — 2026-09-15

A separate runnable local Save Refill release was frozen before changes to the configurator: `Releases/2026-09-15-save-refill`. It defaults off, refills only after accepting the save-station prompt, and passed six native save/cancel/reload cases plus rendered Unreal menu checks.

The [MenuEditor checkpoint](MenuEditor/README.md) records the implemented nine-page editor, complete per-slot request path, presets/import/export, custom logic, supported native additions and verification. Hidden item carriers, fast-door scrolling, the ultra-sparse rainbow-beam counterpart and per-seed HUD/reserve/map choices are connected. Existing world/start/scavenger/general-goal/remaining-patch guards remain: the broader request is **not complete**. The user's local game was not restarted.

## Fast elevators and exact split HUD lists — 2026-09-15

[ElevatorsHud](ElevatorsHud/README.md) completes the native fast-elevator option and versioned per-placement HUD counting from VARIA's actual split-list writer. Five seeds, native movement/counting probes, 25 live oracle queries and Unreal profile checks pass. Native/Game/Editor builds and atomic future-launch staging are complete; the frozen Save Refill hashes remain unchanged. No new local launch occurred. The full native integration goal remains active, including world/start, ordered Scavenger, general objectives and remaining patches.

## Alternate-start foundation — 2026-09-15

[Starts](Starts/README.md) records all 15 start definitions, 44 dependencies and the new reversible native data layer for 40 reviewed non-executable patch definitions. Forty-three full-ROM transaction/rollback scenarios pass on gaming-pc. Native compilation passes. This is a foundation: custom start commit, save PLMs, effective forced settings, executable dependencies, tracker roots and gameplay proof remain required. Generation guards remain enabled; this test library has not replaced the user-facing ElevatorsHud runtime. No local restart and no change to the frozen release.

## Per-slot native starts integrated — 2026-09-15

[Starts](Starts/README.md) now connects native start/world descriptors through generation, effective forced settings, immutable A/B/C plans, initial autosave and later-station reload. Thirteen unshuffled-world seeds pass fresh solver/progression checks and 65 live tracker queries; all 15 native start definitions load in controlled fixtures. Three native banks cover different region/station/Ceres paths, rollback/copy/clear, and later saves. Unreal profile checks pass for all 13 plans; three offscreen start captures were inspected. Ordered world-data restoration and an append-only catalog history preserve older seed interpretation. The two area-only starts and the remaining full-goal world/objective/patch work remain incomplete.

## Native VARIA tweaks — 2026-09-15

[Tweaks](Tweaks/README.md) connects Bomb Torizo pickup awakening and the Lower Norfair Chozo bypass, completing the three-member VARIA tweaks group. Four generated seeds, 20 live queries, 84 native routine cases, independent-bank reload/rollback and 17 Unreal profile fixtures pass. Code-only patch IDs install no SNES instructions and activate only with the committed world plan. Historical catalogs and the Save Refill release are preserved. Door indicators remain blocked pending room integration; their exact source audit proves they are native-interpretable PLM data. Continue the full scope; this is not full VARIA parity.

## Door indicators and full layout group — 2026-09-15

[DoorIndicators](DoorIndicators/README.md) now implements generated per-slot indicators, room insertion/replacement, original PLM animation and X-ray compatibility. Four generated seeds (including all layout plus tweaks), 20 oracle queries, all 18 templates/16 PLMs, native banks and 21 Unreal profile fixtures pass. Actual Vulkan on/off captures show the original assets in the same room. The full layout group is enabled; randomized topology and the remaining full objective are still outstanding. Historical catalogs, source repositories and the Save Refill release are preserved; no local launch.

## Boss connections and shared topology — 2026-09-15

[Connections](Connections/README.md) integrates native boss routing with generation, per-slot world plans, original door/arrival behavior and the shared solver/tracker graph. Four generated boss seeds pass 100-check solver verification and all 400 live tracker progression checks. All 24 permutations, 32 controlled native arrivals, state-specific Wrecked Ship dependencies, 50 world transactions, two boss banks, a historical bank and 25 Unreal profile fixtures pass. A real Vulkan arrival was rendered and inspected. Native/Game/Editor builds passed; no local launch. Full area/door/Minimizer/Scavenger/objective/Tourian/escape/race and remaining patch parity are still incomplete, including VARIA portal map presentation.

## Native randomized door colors — 2026-09-15

[DoorColors](DoorColors/README.md) connects 58 door identities and their effective requirements through generation, native PLMs/graphics, per-slot persistence and solver/tracker validation. Four seeds pass 100-check completion checks and all 400 live tracker progression steps. Native tests cover 300 hit cases, all beam/indicator orientations, missile-only red doors, permanently grey doors, restored Vanilla data and saved door bits. Two banks, the historical bank, 50 world transactions and 29 Unreal profile fixtures pass. The generated Plasma door was rendered in Unreal and inspected. Native/Game/Editor builds pass; no local launch. The original full area/Minimizer/Scavenger/goals/Tourian/escape/race/patch objective remains active, including native map door/portal presentation.

## Area routing foundation — 2026-09-15

[Areas](Areas/README.md) implements a separate 32-AP native catalog, all 1,024 directed door descriptors, original arrival/scroll behavior, 6,509 dependency data bytes, 27 room PLM templates, the two extra Maridia saves and native Tourian elevator refill. Native compilation, all descriptor/setup cases, 128 destination-loader cases and 12 original collision/door transitions pass. The room fixture now waits for the complete save-load arrival fanfare, preventing its earlier audio-queue artifact. All 24 boss permutations and 50 world transactions pass against the same final candidate; representative native captures were inspected. Area generation, complete initial-door plans, the two area-only start integrations, per-slot metadata, solver/tracker wiring and portal exploration/presentation remain outstanding. This candidate is not staged to the user's runtime, the frozen Save Refill release remains intact, and no local game was launched.

## Area generation and initial-door integration — 2026-09-15

[Areas](Areas/README.md) now connects the 32-endpoint graph through generation, solver/tracker contracts, Unreal parsing and independent A/B/C activation. Six area/light/combined/baseline seeds pass fresh 100-check completion verification and all 600 live tracker progression steps. Three native banks, a historical bank and all 15 explicit initial-door definitions pass, including both area-only starts. Legacy seeds retain the earlier initializer semantics. Native/Game/Editor builds pass. Fifteen collision-driven transitions using generated worlds, 35 Unreal profile fixtures and three inspected Vulkan captures also pass. Native portal exploration and map presentation remain incomplete. This candidate has not replaced the future-launch package. No local launch; the three Save Refill release hashes and pristine source repositories were reverified unchanged.

## Original VARIA portal and door sprites — 2026-09-15

[MapIcons](MapIcons/README.md) adds original VARIA map/minimap sprites from `pause_extra.gfx`, with the committed area/boss/door requirements, source exploration visibility and opened-door bits. All 3,394 native pixel comparisons, tracker/check-color regression, a mixed A/B/C bank and two inspected Unreal Vulkan pause runs pass. Native/Game/Editor builds pass. One render fixture reveals all exploration; the other is a fresh map and shows unknown gray portals. Neither proves natural traversal of relocated display tiles. Their exploration hooks, tile counting/map-data parity and later objective icons remain unfinished. Only the isolated gaming-pc runtime was updated; no local launch and no change to the frozen release. The full original goal remains active.

## Native map exploration and layout regions — 2026-09-15

[MapExploration](MapExploration/README.md) connects all eight relocated portal discoveries, original minimap/boss marks, eight special slopes, the missing Fireflea map tile and layout-specific exploration counters. Counts derive from persisted map bits, with a versioned spare-field extension for the one portal bit omitted by original SRAM packing. Both 1,262-tile ownership sets, 522 room/layout assignments, all 1,024 portal combinations, independent exploration slots and historical handling pass. Forty collision-driven transitions across five generated seeds preserve their portal bits/counts after native save/reload. Original sprite/tracker/mixed-bank regressions and an inspected Vulkan arrival/pause/navigation/resume pass, with zero emulated CPU opcodes. Nine room-region assignments now follow the committed layout for HUD/statistics. Only the isolated gaming-pc runtime changed; frozen release/source repositories are preserved. Modified-world exploration masks, objective/boss icons and the full mirror/Minimizer/Scavenger/objectives/Tourian/escape/race/remaining-patch scope remain required.

## General-objective event foundation — 2026-09-15

[Objectives](Objectives/README.md) now records native, persistent population/interactions evidence for general VARIA goals: 145 unique enemy entries, six family counts, room-completion flags, red fish/orange geemer/Shaktool/King Cacatac, bowling Chozo and animal visits. The versioned ZOE1 bitmap preserves original SRAM offsets and is disjoint from ZME1; its event/population identities are locked against silent reinterpretation. All 145 deaths/repeats, 44 exact original-AI comparisons, three real room populations, A/B/C saves and historical guards pass. The final candidate also passes map/exploration, 3,394 sprite comparisons, tracker/mixed-bank regression, 40 generated-world transitions and an inspected Vulkan pause run, with zero emulated CPU opcodes. This does not yet enable general goal generation/evaluation, the objectives screen or alternate endgame gates. Continue the full scope through a per-slot effective objective descriptor, native evaluation and solver/tracker/UI parity, then the remaining world/Scavenger/Tourian/escape/race/patch work. No local launch or frozen-release change.

## Native objective conditions and ordinary Tourian gate — 2026-09-15

[Objectives/Conditions](Objectives/Conditions/README.md) adds the versioned pending/applied per-slot objective descriptor, all 58 supported condition evaluators, one-goal-per-frame latching, required/all-complete flags, source SFX batching and hidden reveal. Native A/B/C save/reload and actual statue-room collision/animation tests pass: defeating G4 cannot bypass an unmet configured goal, while completing a non-boss goal opens the ordinary passage with no defeated bosses. Existing event/map/sprite/tracker/bank regressions and an inspected Unreal Vulkan pause run pass against the final candidate. A source-writer capture module is tested on three fresh generated pools but is not yet connected to production generation. Required remaining work: manifest/Unreal activation, objective-aware solver/tracker, original goal UI/icons, then the full ordered Scavenger/modified-world/Minimizer/Tourian/escape/race/remaining-patch scope. Generation guards remain, no local launch or package replacement, frozen release and pristine upstreams preserved.

## Objective generation/profile binding — 2026-09-15

[Objectives/Binding](Objectives/Binding/README.md) connects effective source-writer objective contracts through seed fingerprints, matching tracker metadata, Unreal parsing and native generation/A/B/C activation. Vanilla, relic and historical plans retain their previous rules. Four generated seeds pass fresh 100-check solver completion and all 400 live tracker progression checks; three mixed native banks and 38 Unreal profile fixtures cover rollback, autosave, reload, copy/clear and old-plan compatibility. New native goal progress survives actual save/reboot/load independently in each slot. The public frame entry now advances one objective once per frame, correcting its earlier double helper call. Native/Game/Editor builds, all 58 condition cases and an inspected Vulkan run with the new contract pass. Nondefault goal controls remain guarded until general solver/tracker semantics and original goal UI/dependencies are complete. Continue the entire Scavenger/modified-world/Minimizer/Tourian/escape/race/remaining-patch scope. No local launch/package staging; frozen release and pristine upstreams preserved.

## Objective-aware solver and native tracker evidence — 2026-09-15

[Objectives/Logic](Objectives/Logic/README.md) replaces hardcoded G4 assumptions with effective goal identities/quota, source-written collection memberships, equipment masks and enemy totals. The fresh solver now records objective visits and quota attainment separately from the stable item progression log. Seven generated goal pools pass complete 100-check routes and all 700 live tracker item steps, plus source objective steps and collection boundaries. Native evidence travels through a versioned companion to the unchanged tracker ABI; feasible interactions remain distinct from actual persistent completion. Stale custom-event/slot/session results are rejected. All 58 native conditions, three-slot condition persistence, native-to-embedded-oracle save/reload, 45 Unreal profile fixtures and the actual asynchronous Unreal tracker path pass. The original pause map was rendered and inspected under Vulkan. Native/Game/Editor builds pass. Public nondefault goal generation stays guarded pending original objective UI/notifications/icons and remaining dependencies; this is not full objective parity. Continue those, then the original Scavenger/modified-world/Minimizer/Tourian/escape/race/remaining-patch scope. No local launch or package replacement; frozen release and pristine upstreams are preserved.

## Native objective page and persistent HUD notifications — 2026-09-15

[Objectives/Presentation](Objectives/Presentation/README.md) adds the original VARIA objective page to the native pause state machine, with widescreen reflow, hidden reveal, source progress rules and eighteen-entry scrolling. Original footer glyphs and shoulder animations are preserved; map/equipment graphics restore exactly. Effective goal ranks and quota now drive 300-frame HUD notifications with original per-slot persistent acknowledgement events. Native navigation, 320 footer checks, eighteen notification ranks, A/B/C saves, 58 conditions, live oracle consistency, 3,394 map sprite comparisons and the current historical HUD/reserve suite pass. Two previously generated goal seeds pass actual asynchronous Unreal tracking and original pause navigation under Vulkan; final captures were inspected. Native/Game/Editor builds pass. Nondefault generation remains guarded pending objective map icons and BT/SFX dependencies. Continue those and the full unchanged Scavenger/modified-world/Minimizer/Tourian/escape/race/remaining-patch scope. Only the isolated gaming-pc runtime changed; no local launch or package replacement, and frozen release/pristine upstream hashes are preserved.


## General objective controls, source map sprites and dependencies — 2026-09-15

[Objectives/MapDependencies](Objectives/MapDependencies/README.md) enables the production general-objective controls for full original/area-randomized worlds with ordinary Tourian. Manual/random goals, source count/quota distributions, category distribution, hidden goals and explicit `nothing` selection now use effective per-slot contracts. Requested settings remain separate; forced map/HUD and effective objective values are saved accurately. Original numbered map sprites follow VARIA's overlap, completion and hidden rules, with 648 source-writer/pixel cases. Source-written Bomb Torizo sleep and translated-SPC priority are implemented and tested.

Nine production seeds pass 100-check solver/objective progressions and all 900 live tracker item steps; native embedded generation reproduces their exact fingerprints and tracker metadata. A real sparse seed records Bomb Torizo sleep. All 58 native conditions, historical HUD/reserves, 3,394 existing map sprites, native pause/notifications and A/B/C persistence pass. Fifty-four Unreal profile fixtures and two inspected Vulkan pause/oracle runs pass. Native/Game/Editor builds pass; only the isolated gaming-pc runtime was staged. Frozen release and pristine upstreams remain unchanged, with no local launch or package replacement.

The full goal remains active. Next is ordered Scavenger: source-written order, actual pickup restrictions and Ridley handling, original HUD list browsing, persistent position, source solver ordering and integrated tracker eligibility. Continue the original modified/mirror/Minimizer worlds, alternate Tourian/escape/race and remaining non-cosmetic patches; this checkpoint is not complete VARIA parity.


## Scavenger integration — 2026-09-15

See [Scavenger/README.md](Scavenger/README.md). Public requests now generate exact source-written orders; native pickups/Ridley, original HUD browsing, goal progress, A/B/C persistence, solver and tracker share that order. Three seeds (4/8/17 mandatory locations, area/boss variants) pass all 100 source-solver checks and ordered progression, exact native-API reproduction, targeted tracker boundaries, native save/goal routines, 57 profile fixtures and three inspected Vulkan runs. Game/Editor/native builds pass. The release, old save layouts and upstreams remain intact. No local launch or packaged-runtime replacement. Full integration remains unfinished: modified/mirror worlds, Minimizer, alternate Tourian/escape/race and remaining patches.


## Fast Tourian dependency — 2026-09-15

[FastTourian/README.md](FastTourian/README.md) records native source-faithful door routing, the invulnerable quota-controlled eye, Mother Brain fast death/Hyper Beam, authored room data, map-count correction and original pause labels. Objective version 3 composes with Scavenger while preserving old saves/contracts. Three public seeds pass 300 solver/tracker item steps and native-API reproduction; actual native door/MB routines, shortened hallway traversal, 60 profiles and three Vulkan runs pass within their recorded scopes. Final guardian correction has its own native proof; unchanged render proofs retain their actual previous binary hash. Full goal remains incomplete: Minimizer and Mirror, Disabled Tourian/escape/race and remaining gameplay patches. No local launch or packaged runtime replacement.

## Minimizer mixed world — in progress, 2026-09-15

[Minimizer/README.md](Minimizer/README.md) records the new 40-endpoint native
routing, retained-check masks, filtered map/enemy counters, objective schema 4,
solver and tracker semantics, and typed A/B/C Unreal plans. Three internal
42/64/100-check worlds complete their source routes; 206 embedded tracker steps,
120 native arrivals and 63 profile fixtures pass. One inspected Vulkan run
passes the asynchronous oracle and original pause UI. Native/Game/Editor builds pass.
Public Minimizer generation remains guarded pending effective option dependencies,
filtered objective sprites, remaining boss-room audit and production API evidence.
This is progress toward the full goal, not a completed mode or full VARIA parity.


## Public Minimizer integration — 2026-09-15

[Minimizer/README.md](Minimizer/README.md) now enables production requests with
effective area/boss/suit/split dependencies and faithful random target bounds.
Three new seeds (30/72/100 effective checks, exploration and Scavenger variants)
pass complete solver routes, exact native-API reproduction and 202 live tracker
steps. Four original boss-death seams, the boss-room save PLM, 120 native arrivals,
54 filtered objective-sprite comparisons and one original collision-driven mixed
transition pass. The public plans pass 63 profile fixtures and an inspected
Unreal Vulkan run; Native/Game/Editor builds pass. Earlier internal proofs remain
separate. No local launch or packaged runtime replacement; frozen release and
upstreams unchanged. The full goal is still incomplete: Mirror, Disabled
Tourian/escape/race and remaining noncosmetic patches.

## Escape common native behavior — in progress, 2026-09-15

[Escape/README.md](Escape/README.md) implements source-written static/regional
clocks, quota-triggered escape, timer/earthquake/music, save refusal, Hyper Beam
block rules and exact enemy/elevator population handling. Three source captures,
30 native timer starts and 48 enemy-header cases plus native trigger/save/block
and restoration checks pass. No public option activation: ordered routing,
animal cycles/room/map data, objective schemas, solver/trackers and Unreal remain
required. The native candidate is isolated; no local launch or package change.


2026-09-15 escape routing: [Escape/README.md](Escape/README.md) now records ordered source-writer routing, native animal cycles and room setup. Reused captures for three seeds pass 31 effective doors, 24 cycle exits and independent slot/restoration checks on gaming-pc. Production escape remains guarded. Next: remaining WS level/map data, production topology/objective/solver/Unreal binding and full-seed runtime proof. The complete Mirror/escape/race/remaining-patch goal is unchanged.


2026-09-15 public escape binding: [Escape/README.md](Escape/README.md) enables public escape/Disabled Tourian with typed per-slot plans, source WS/map data, filtered counters and correct solver/ship completion semantics. Three public seeds match the tested internal contracts; 267 embedded tracker steps, native schema-5/WS/count/rollback checks, 66 profiles and one inspected Vulkan run pass. Game/Editor build. Remaining mode gates: collision-driven escape and ship ending, Fast/Scavenger combinations; full Mirror/race/remaining-patch scope stays active. No local launch or packaged-runtime/release replacement.


2026-09-15 escape transition/ending evidence: three existing public seeds now pass actual native collision-driven door changes. Disabled Tourian reaches real credits through ship input/AI/takeoff after a controlled quota and return fixture. One new Escape + Fast Tourian + Scavenger + light-area seed passes complete source progression, 100 embedded tracker steps and native combined-plan/count checks. See Escape/README.md for precise scopes. Next: Mirror, race and remaining noncosmetic patches; full goal remains active.

2026-09-15 gameplay patch checkpoint: native implementations for relaxed round-robin Crystal Flash, momentum-preserving landing and nerfed Charge compile successfully in `.tmp/escape-native` (log `.tmp/gameplay-build.log`). This is compilation evidence only: targeted runtime checks and public option/manifest/Unreal bindings remain pending. Do not mark these patches integrated or reuse the new candidate as the previously validated Escape binary. Preserve existing Escape proofs and run only checks affected by the new changes. User requests reduced usage with 26% allowance remaining; avoid repeating completed suites or expanding scope. Mirror, race and remaining noncosmetic patches are still outstanding.

2026-09-15 gameplay patches integrated: [GameplayPatches/README.md](GameplayPatches/README.md) supersedes the preceding compilation-only checkpoint. All three options are wired through generation, native rules, manifest validation and independent persisted slot plans. Targeted native routines, two new complete solver routes, 200 embedded tracker steps and focused Unreal A/B/C persistence checks pass on gaming-pc. Native/Game/Editor builds pass. Frozen release and upstreams verified unchanged. Mirror, race and other remaining noncosmetic patches are still outstanding; the full goal remains active.

2026-09-15 suit modes integrated: [Suits/README.md](Suits/README.md) records native Vanilla/Balanced/Progressive heat, fractional environmental, enemy and Metroid damage. Public options, manifests, per-slot activation and tracker/source logic agree. Three complete seeds, 300 embedded tracker steps, 24 native equipment/session combinations, 96 fractional damage cases and focused A/B/C/legacy profile checks pass. Native/Game/Editor builds pass. Old tested seeds were Vanilla, not Balanced; they remain Vanilla. Remaining guarded choices are Mirror, raceMode, itemsounds, spinjumprestart, Infinite_Space_Jump and animals. Full goal stays active.

2026-09-15 movement/pickup options integrated: [MovementPickups/README.md](MovementPickups/README.md) closes respin, Infinite Space Jump timing and all original item sound choices. Source data, native routines, public generation, per-slot rules, profiles and embedded tracker are connected. 4,040 pose lookups, 96 Space Jump cases, 63 sound mappings, two complete solver routes and 200 tracker steps pass on gaming-pc. Native/Game/Editor builds pass; no local launch or package replacement. Remaining guarded options are Mirror, raceMode and animals. Full goal remains active.

2026-09-15 Mirror first native increment: [Mirror/README.md](Mirror/README.md) records 75 byte-verified source scroll routines, pending/applied slot integration and 225 actual native dispatch checks. A separate source geometry catalog verifies all 100 Mirror checks/map coordinates, preserving collection IDs and identifying two relocated PLMs. Native build passes. Public Mirror stays guarded pending geometry/code/AI, flavor-aware contracts and full seed/runtime validation. Main test runtime stays on the completed MovementPickups candidate; Mirror candidate remains isolated. Race/animals and the full goal remain outstanding.

2026-09-16 Mirror geometry checkpoint: source data installation/restoration and
physical item resolution are native, with canonical seed/save identities kept.
385,906 bytes, all 100 checks, full ROM slot restoration and ten room/treadmill
callback cases pass on gaming-pc. A controlled $9E9F room loads in 98 native
frames and runs 120 more with an inspected C-core image, zero emulated opcodes.
See Mirror/README.md and new Proofs. Public Mirror remains guarded: native AI,
variant topology/starts/maps/objectives and full generated-seed gameplay remain.
Race and animals remain outstanding. No public runtime or release replacement.

2026-09-16 Mirror enemy/room checkpoint: native Boulder/Etecoon changes and
lava/statue PLMs implemented, with authored animation/velocity data and C
literal copies handled. 82 targeted native cases pass on gaming-pc. Source
code-range ledger prevents redoing reviewed callbacks; 733 pending patch
records remain (not a function count). See Mirror/README.md for exact limits.
Full Mirror, Race and animals remain outstanding; public Mirror stays guarded.

2026-09-16 Animals Surprise integrated: all ten source variants have native data
and callbacks, actual source random draw capture, typed manifest validation,
per-slot generation/activation and profile persistence. Ten-variant native
checks, three public solver seeds, 200 embedded tracker checks and focused
Unreal profile tests pass. Native/Game/Editor build. Source ignores animals
with Escape Randomization and that effective setting is preserved. See
Animals/README.md for controlled-test limits. Remaining guarded modes: Mirror
and Race. Full goal remains active; no local launch or frozen-release change.
