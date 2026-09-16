# Native map and item trackers

Implemented on 2026-09-15. The tracker is part of the port: item squares and boss diamonds are drawn onto the **original pause map and HUD minimap**. The native map retains its tiles, scroll, player marker, boss icons, area label and controls. The equipment screen remains native. The [original Area Select screen](AreaSelection/README.md) is also available from pause for browsing other regions and their live check counts.

## Controls and defaults

Open the game map with **Start**. Press **Select** for the original region overview, choose a region with the D-pad, and press **A** to view it; **B** returns. The map header displays the pack's original item and boss icons, with acquired capacities. Equipment is dim until acquired; boss portraits dim after defeat. Disabling equipment in the Samus screen does not mark it as missing. Ammo counters show maximum rounds, while energy and reserve counters show tank counts.

In the English system menu, **Settings → Interface** contains:

- **Trackers in Vanilla**: off by default; enables the same display and complete authored map, using original placements and unpatched vanilla logic.
- **Map tracker** and **Item tracker**: separate display switches, on by default for randomized slots.
- **Vanilla tracker techniques**: Casual / Regular / Veteran. Randomized slots always use their own saved effective configuration.
- **Tracker legend** and the live logic service status.

Display preferences are shared local settings in `Presentation.ini`. Inventory, check collection and seed context belong to the selected native A/B/C slot. Nothing is copied between those slots by the tracker. The complete map is presentation knowledge: actual exploration and map-station save flags remain genuine. Enabling/disabling Vanilla tracking while already paused reloads the native map immediately.

See [Boss accessibility markers](BOSS-MARKERS.md) for combat logic, colors, and the Spore Spawn regression.

## Runtime independence

**Neither PopTracker nor its pack is a runtime dependency.** `Native/sm_tracker_assets.inc` contains the incorporated icon pixels and the audited 100 location coordinates. It is compiled into `libsm_native.so`; no Lua, pack JSON, PNG or atlas is loaded from `Docs/References/PopTracker/pack` at runtime. Building the library also uses this existing include directly. `Scripts/build-tracker-assets.py` is an optional developer tool to regenerate it from the preserved reference assets.

The only logic engine is the port's existing embedded Python/VARIA runtime. `Randomizer/sm_live_tracker.py` uses the same pinned local `Randomizer/upstream` as generation and `Randomizer/native_locations.json`. There is no PopTracker process, Archipelago connection, emulator-memory connection or separate tracker window. Runtime staging excludes the reference pack and includes the relevant notices.

## Effective logic

1. `sm_tracker_snapshot` copies the actual native acquired equipment, beam bits, ammo/tank capacities, 100 location flags, bosses, opened doors, exploration and identity. Acquired items and location collection are independent.
2. `FSMTracker` detects meaningful changes, serializes the effective settings plus inventory, and runs an asynchronous query. Samus movement and frame counters do not cause a recalculation.
3. `sm_tracker_evaluate` shares the embedded generator's mutex. Every request receives a fresh Python subinterpreter, isolating VARIA's mutable globals, techniques, doors and caches from other seeds and generation.
4. VARIA evaluates access from Landing Site with the saved expanded `Knows`, hard rooms, hell runs, boss difficulty, effective logic patches, door colors and numeric difficulty ceiling. The selected seed supplies its validated vanilla, area, boss, or Minimizer topology. Opened doors and defeated bosses/minibosses come from native flags.
5. Return checks use tracker mode: the unknown object at the destination is **never borrowed** to justify escaping. Return paths must also fit the configured difficulty ceiling for green. A reachable location with uncertain return, or an enabled route above the ceiling, is yellow. Disabled techniques are not granted by the tracker.
6. The game thread accepts results only for the matching profile context, native slot, seed fingerprint, session and inventory. A changed inventory or new load rejects stale results. Pending results use neutral white outlines, not fabricated accessibility. Already-collected locations clear immediately.

Progression speed changes the generated placement; it is not an extra traversal lock. Spoiler placements are deliberately absent from the request sent by Unreal. The standalone query also ignores any supplied spoiler fields. Movement assists do not grant items or silently enable techniques.

## Exact map colors

The reference pack inherits these defaults from [PopTracker's MapWidget](https://github.com/black-sliver/PopTracker/blob/7971763e4ff046d880232a45c1fc4940c0eb43f9/src/ui/mapwidget.cpp). These are RGB values before the original screen fade; the GUI bypasses the atmosphere/blur layer.

| State bits | Meaning | RGB |
| --- | --- | --- |
| 0 | Collected | `#3F3F3F` |
| 1 | Accessible, return within rules | `#20FF20` |
| 2 | Blocked | `#CF1010` |
| 4 | Advanced route / return not guaranteed | `#FFFF20` |
| 8 | Inspect only | `#3040FF` |

The current VARIA integration emits no inspect-only state: knowing an item exists in a secret tile does not prove it can be inspected. Blue remains supported in the native renderer for actual inspect results. Multiple checks at the same native map cell use PopTracker's default split triangles, combining the remaining checks' colors; collected checks do not contribute a blocked state. The complete 16-entry mixed-color palette is exposed by `sm_tracker_color` and verified in the native test.

Coordinates come from original room headers and item PLM positions, not scaling the pack's map PNG. The pause map uses 8-pixel cells, native BG1 scrolling, and the 72-pixel widescreen extension. The expanded minimap uses its actual 7×4 viewport; the original 5×3 viewport receives the same check state. Markers clip to the visible map interior. Small collected outlines and a visible player center keep the underlying map readable.

## Assets and attribution

Original item and boss PNGs: Cyb3R's SM PopTracker pack, MIT, notice in `Native/TrackerAssets-LICENSE.txt`. The original equipment artwork is sampled from its 3× source pixel grid; boss portraits are reduced from 32×32 to compact 16×16 icons with nearest sampling. No AI-generated artwork, smoothing or replacement map art is used. `assets.json` records each source hash. The pack credits The T for its reference maps; those PNG maps are **not** used to draw the game map.

Color and triangle reference: black-sliver/PopTracker commit `7971763e4ff046d880232a45c1fc4940c0eb43f9`, GPL-3.0; notice in `Native/TrackerPalette-LICENSE.txt`. The UI asset geometry remains the original SM ROM geometry.

## Validation and limits

Tests run headlessly on **gaming-pc**, under `/tmp/sm-native-generation-20260914`, with its required `ISOLATED_TEST_DIRECTORY` marker. This directory has **no reference pack**. No installed game or user save is modified and no interactive game window is opened.

- `Scripts/test-live-tracker-logic.py`: 316 embedded queries, three existing independently verified seeds, all **100 progression steps per seed**, full inventory, effective technique changes, location-vs-item identity, spoiler independence and return to the Vanilla baseline after randomized contexts.
- `Scripts/test-live-tracker-native.py`: actual native game/room/pause frames; pause and minimap colors, acquired-vs-equipped handling, stale snapshot/session rejection, randomized simulation RAM parity, original Vanilla placements and map-toggle behavior. Includes rendered captures and the unmodified source ROM hash.
- `SMTrackerSelfTest` in the actual Unreal game binary: asynchronous `FSMTracker`, native ABI and real embedded Python runtime, with controlled inventory fixtures for Vanilla and two randomized contexts; inventory refreshes and a pending query during shutdown are covered.
- `Scripts/test-tracker-generation-serialization.py`: concurrent generation/query submissions use the shared runtime safely; the regenerated seed retains its exact fingerprint and the Vanilla query retains its original rules.
- Native library, Unreal game and Unreal Editor builds complete separately.

The progression replay proves logical consistency with the seed solver, not a controller-driven playthrough. Native captures prove the tracker pixels and their integration with the native map; the Unreal headless test proves the host/controller path, not the final Vulkan material composition or interactive readability. That final visual/play session remains for the user's requested launch.

## Implementation checklist

- [x] Effective VARIA rules, without spoiler item borrowing.
- [x] Original native map and minimap markers with PopTracker colors.
- [x] Original item icons and acquired capacities in the map header.
- [x] Randomized defaults, explicit Vanilla opt-in and settings persistence.
- [x] Slot/session isolation, asynchronous updates and stale-result rejection.
- [x] Pack-free headless runtime validation, source attribution and build integration.

Evidence: [logic](logic-verification.json), [native frames](native-verification.json), [shared runtime](serialization-verification.json), [Unreal controller](unreal-selftest.log), [builds and hashes](build-evidence.json).

![Native pause map with live checks](on-pause.png)

![Acquired inventory and accessible checks](all-items-pause.png)

![Checks inside the gameplay minimap](on-minimap.png)
