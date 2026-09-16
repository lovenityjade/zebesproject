# General objectives: map assets, dependencies and public generation

The development configurator now accepts general VARIA objectives for a full original/area-randomized world with ordinary Tourian. Manual goals, eligible random pools, random counts, required quotas, category distribution and hidden goals use the production request path. The original native evaluator, pause page, notifications, source solver and integrated trackers consume the effective per-slot contract. Ordered Scavenger, modified/mirror/Minimizer worlds, alternate Tourian, escape/race and remaining gameplay patches are still required for the full project goal.

## Original map presentation

`Scripts/build-objective-map.py` extracts the eighteen number sprites, authored colors and sprite offsets from the pinned VARIA assets. Its 58 named points occupy 57 unique pixel positions, including half-tile offsets. `map-audit.json` records source hashes, memberships, categories and pixels.

The native pause map follows `RomPatcher.writeObjectivesMapIcons`: a more specific goal in the same category owns an overlapping position; a later Memes-category goal wins across categories. Completion hides that owner without exposing a lower-priority fallback. Goal and sub-event completion, hidden reveal, map scrolling and clipping apply to the original number sprites. Original boss boxes are suppressed on this pause map. These numbers are not added to the gameplay minimap, matching VARIA; existing integrated item/check markers remain available there.

The renderer writes the 256-pixel UI and 400-pixel widescreen overlay using the same source pixels. It does not generate new art or require the PopTracker pack.

## Native dependencies

The source objective writer determines Bomb Torizo sleep from the actual Bomb pickup, applied awakening tweak and Chozo-robots goal. Capture runs this writer in temporary data storage, then restores both FakeROM data and ROM-option values. Native flag 4 stores that effective decision in the existing objective descriptor. The awake/sleep branch is enforced in the actual native door/statue routines. A sleeping Torizo plus an active robots goal is rejected. Original SRAM and descriptor layouts are unchanged; historical plans without the flag retain their behavior.

The objective sound uses the original two SPC voices with priority one, corresponding to VARIA's sound-table target at $39A8 instead of $399D. The translated native SPC handles this directly; executable patch bytes are not installed. Competing ordinary effects cannot cut off this notification. Vanilla and historical plans without an objective contract keep the original priority.

## Production request semantics

The resolver no longer rejects general objective options already implemented natively. `nothing` is selectable in the manual list under **No additional objectives**, while remaining excluded from the random pool. Its original exclusion constraints apply.

Random objective and required counts are left to VARIA's own bounded distributions, after it knows the available goals. `requestedSettings` retains the exact request. `rules.options` records the effective goal list, count, quota, visibility and forced HUD/map settings; `nativeContext.objectives` remains the authoritative runtime contract. Manual selection disables hidden/distributed goals as VARIA does. The historical effective contracts are never rewritten.

## Validation checkpoint

The following native reports have passed on the isolated gaming-pc candidate:

- `map-results.json`: 648 full-frame source-writer/pixel comparisons, all supported goal memberships, all eighteen ranks, both orders of overlapping owners, hidden/reveal, completion, clipping and Vanilla boss-box gating.
- `dependencies-results.json`: native door/statue/item-deletion routines, pending/committed A/B/C flag isolation and rollback, plus actual translated-SPC voice/priority/competing-sound behavior and non-silent PCM captures.
- `public-seeds/verification.json`: nine public production requests, without a resolver override, each verified for 100 reachable checks and objective progression. The cases include default expansion, hidden distributed area goals, forced map/HUD dependencies, Chozo split, randomized boss/door requirements, no additional objectives, sparse item counting, a real generated sleeping-Bomb-Torizo seed, and random list/quota counts.
- `tracker/verification.json`: all 900 item steps plus source objective steps through the embedded oracle. This proves solver/tracker consistency, not nine manual playthroughs.

Final validation also passed:

- `regressions.json`: 58 native conditions, stale-safe native/embedded oracle publication, 3,394 existing door/portal sprite comparisons, historical HUD/reserve behavior, pause navigation/320 footer checks, eighteen HUD ranks and A/B/C persistent notifications.
- `public-embedded.json`: all nine exact requests reproduce the same seed fingerprints, native contracts and tracker metadata through `sm_randomizer_generate`, the C API used by Unreal.
- `profile.log`: 54 generated/historical plan fixtures pass publication, reload, independent slot copying and malformed-plan rejection, including the sleeping-Torizo flag and new public objective requests.
- `render-results.json`: seeds 15093001 and 15093002 pass actual asynchronous Unreal tracking, map/objective/equipment navigation and direct resume under Vulkan. `01-objective-pause-200.png`, `02-objective-pause-100.png` and `02-objective-pause-200.png` were inspected. The hidden hint, visible counters and original objective number at Bomb render with the authored font/frame/sprites. These are automated native-room fixtures, not complete physical playthroughs.

Native, Game and Editor builds passed. The isolated remote runtime was staged with this candidate and its test processes exited. The first regression driver pointed at the wrong directory for an unchanged pause-art fixture; `initial-missing-fixture.log` retains that setup error. The corrected driver passed the complete suite. The historical HUD report originally hashed the staged runtime rather than the library loaded by the fixture; the test now records `l._name`, and its corrected rerun is recorded separately in `regressions-rerun.json` and `legacy-ui/verification.json`. `build-hashes.json` identifies the candidate; `preservation.json` verifies the frozen release and pristine upstreams. The local game/package has not been launched or replaced.
