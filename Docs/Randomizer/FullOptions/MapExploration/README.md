# Native map exploration and region ownership

Development candidate for the full VARIA integration. The local packaged runtime and frozen Save Refill release are unchanged. The complete randomizer goal remains active.

## Source contract

`Scripts/build-map-exploration.py` compiles 1,262 authored map coordinates from VARIA's `tools/map/graph_area/normal_*.json`, then overlays `alt_*.json` for the original layout, exactly as `tools/map_tilecount.py` does. Both resulting region totals are checked against the pinned upstream `Logic.map_tilecount`. `audit.json` records coordinates, ownership in both layouts, original patch/source hashes, slopes, portal display locations, and room-state region assignments.

Eight source slope tiles also explore the tile directly above, recursively and only when newly visited. The compiler reads `map_data.ips`, `map_data_area.ips` and `map_data_area_alt.ips` in memory to identify these positions. Native tile numbers cannot identify them because VARIA rearranges map graphics. The generated native movement overlay redirects both the original minimap and boss-room exploration writes to `sm_map_exploration_mark`. Ordinary Vanilla exploration is unchanged.

Exactly one counted coordinate is absent from the original map: Green Brinstar Fireflea Room at physical Brinstar `(7,8)`. The native ROM-data view receives word `$CC25` at PC `$1A820E` in randomized sessions. This uses VARIA's shape and the original map palette; the compiler proves tile `$25` pixels match the original game's graphics. Vanilla restores the captured word. This never edits the user's ROM file. Complete-map knowledge is rebuilt after each successful world application, including restoration to Vanilla, so it follows the selected slot.

`sm_map_exploration_portal` is called by the committed native area-door handler after arrival setup. It implements `RomPatcher.writeExploreMapAsm` for all eight relocated endpoints, including source/destination processing and the exact physical-area mirror conditions. The two `coversTile` locations belong to the counted coordinate set; the other six display-only positions do not. Drawing icons still never changes exploration.

## Counters and persistence

`sm_map_exploration_value(region, field)` exposes actual explored counts (`field=0`) and available totals (`field=1`), with graph-region IDs `0..11` or `-1` for the total. Counts read the active exploration mirror for the current physical area and the saved-map buffer for others. They never read map-station knowledge or fully-known-map masks. This makes repeated visits idempotent and derives the history of existing saves without guessing initial counter values. The API is for subsequent native objective evaluation; general objectives are not enabled by this increment.

The supported full worlds have 1,170 counted tiles in the original layout or 1,165 with area randomization. Ceres and Tourian are excluded, as in the source writer for these worlds. Mirror, Minimizer, escape map-station removal and Fast Tourian still require their effective seed-specific masks/totals and remain generation-gated. These constants must not be reused to enable those modes.

The original SRAM packing tables preserve all counted tiles. They omit one display-only bit: East Tunnel Top Right, physical Brinstar byte 201, mask `$80`. `ZME1` plus that bit is stored at offset `$150..$154` of the existing 1,280-byte compressed-map save field, outside the original used bytes `0..326`. Existing offsets, checksums, and map bytes are retained. Saves without the signature get no invented portal history. The standard save/load hooks serialize and restore this extension; counts need no separate saved values. No VARIA count bytes are written into the native door-bit array, so the existing tracker oracle sees only its original door state.

The compiled source room tables also correct nine layout-dependent region assignments. `sm_varia_region()` now selects the committed layout for HUD names and run statistics. Previously recorded statistics are retained; subsequent time uses the corrected region. All 261 rooms and both layouts are covered; none of the nine reassigned rooms contains an item location, so the immutable location counting lists retain their original meaning.

## Validation checkpoint

Native compilation passes. The isolated gaming-pc native suite checks every authored coordinate in both layouts, source totals, all 261 room assignments in both layouts, eight actual native minimap slope calls, all 1,024 relocated-portal combinations with byte-for-byte RAM comparisons, repeated visits, original checksummed SRAM reload, historical saves without the extension, rejected/pending world changes and Vanilla restoration. Emulated CPU opcodes remain zero.

The original 3,394 source-pixel comparisons and existing live tracker/mixed-bank regressions also pass against the candidate. All 40 collision-driven transitions at the eight relocated endpoints pass across five generated worlds, with portal/count save/reload assertions after each arrival. Separate A/B/C exploration patterns and the extension bit survive six out-of-order reloads. Those fixtures prepare source rooms and equipment; they are not manual playthroughs or a new solver verification. The existing Unreal Game binary loaded the new native library under Vulkan. Its generated Red Fish → Green Brinstar Elevator arrival, native pause/navigation and resume passed without the all-map-bits fixture; the map capture was inspected. It shows the correct Green Brinstar graph-region HUD while the elevator remains on the original physical Crateria map. This render uses a prepared arrival; the separate 40 cases prove collision-driven discovery. `Proofs/build-hashes.json` binds the native candidate, regression results and render command to exact artifacts. Native was rebuilt; the unchanged Unreal executable was reused.

## Remaining full-goal work

Continue original map objective/boss-icon parity and effective map masks as the remaining modes are implemented. Mirror/Minimizer, ordered Scavenger, general objectives, Tourian, escape, race and remaining gameplay patches are still required. Do not mark the full goal complete on the basis of this checkpoint.
