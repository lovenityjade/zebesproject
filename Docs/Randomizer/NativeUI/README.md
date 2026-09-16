# Native VARIA UI for randomized games

Implemented 2026-09-15. These are native C behaviors and original VARIA HUD tiles, integrated into the existing game HUD, pause map and minimap. No IPS code is executed and the source ROM is unchanged.

## Defaults and controls

Every Randomized A/B/C slot uses these options by default, including previously generated seeds. Vanilla slots do not activate them. English **Settings → Interface** exposes four independent switches:

| Setting | Native behavior |
| --- | --- |
| VARIA maximum ammo | Maximum capacity above the original compact weapon icons, current ammo below; three-digit Supers and Power Bombs, selection and auto-cancel highlighting. |
| VARIA area & objective HUD | VARIA graph-region abbreviation and remaining uncollected locations, seven tiles representing fourteen energy tanks, five-second boss-objective notifications. |
| VARIA improved reserves | Full reserve pickups; automatic/manual transfer consumes only needed energy; A cancels manual transfer; selecting away cancels it; automatic transfer preserves invincibility and suppresses periodic heat damage; empty/partial/full AUTO indicator. |
| VARIA map item markers | Neutral collection markers on the native pause map and minimap when the logic map tracker is disabled. The existing PopTracker-colored logic squares take precedence when enabled. |

The randomizer's Patches page links to these settings with **Configure VARIA interface**. Preferences are shared local `[VariaUI]` values in `Presentation.ini`, independent of a seed's immutable item-placement fingerprint. Generation's `patches` list is a catalog declaration, not a setter for these preferences. New seeds need no additional step, and old seeds need no regeneration. Inventory, collection and boss state remain specific to the selected native slot. This does not enable VARIA features in Vanilla; the separate Vanilla tracker opt-in remains available.

## Fidelity and supported rules

The full-pool counter reads real location collection flags, not acquired equipment. Regions come from VARIA's original room graph: for example West Ocean belongs to Wrecked Ship, and the Morph room belongs to Crateria. This is intentionally different from the game's six broad map areas.

The current integrated generator uses Full / 100 locations, original topology and the four major bosses. Notifications use those actual boss flags, followed by the all-objectives message. Already-completed bosses do not replay notifications after a load. Custom objective sets, Major/Chozo splits and Scavenger are still unsupported generation modes; their HUD variants are not claimed implemented.

The original native map tiles, scrolling, assets and existing full-map knowledge remain in use. **Collection markers are a native equivalent, not the whole upstream Map Overhaul.** Door/area-randomizer maps, map-area switching enhancements and VARIA backup-save mechanics are not included here.

The authored `map/hud.gfx`, literal graphic edits in `max_ammo_display.asm`, and `tables/hud_chars.txt` are compiled into `Native/sm_varia_assets.inc`. `Scripts/build-varia-ui-assets.py` regenerates that include for developers. Runtime and ordinary builds do not read these reference graphics, and no PopTracker pack is required. The render layer preserves the previously requested transparent HUD background and bypasses scene blur/atmosphere, in both 256- and 400-pixel presentations. Item-message graphics are not overwritten by the compact HUD icons.

## Implementation and attribution

- `Native/sm_varia_ui.c`: mode guard, counters, notifications, reserve routines and native glyph rendering.
- `Native/prepare_overlays.py`: exact-count hooks in generated bank 82/84 copies; pristine `native-core` is preserved.
- `Native/sm_scene.c`: captures the HUD palette before scanline IRQ changes.
- `Native/sm_tracker.c`: collection-marker fallback and tracker precedence.
- `Unreal/Source/SMUnreal/SMSystemMenu.cpp`: persistent English controls.
- `Randomizer/patches.json`: native implementation status and Randomized-only activation.

Sources: pinned `Randomizer/upstream/patches/common/src/max_ammo_display.asm`, `varia_hud.asm`, `better_reserves/main.asm`, `better_reserves/hud.asm`, and `map/`. Maximum-ammo work credits Personitis, theonlydude and maddo; reserve improvements credit Nodever2 and Benox50; VARIA HUD and map assets credit the upstream contributors. The upstream MIT notice is reproduced in `Native/VariaUI-LICENSE.txt` and included in runtime staging. No AI-generated artwork is used.

## Verification

Headless checks run on **gaming-pc** in the marked isolated directory `/tmp/sm-native-generation-20260914`, without the reference pack, installed-game changes or user-save writes. Fixtures are deliberately instrumented native states, not a full controller playthrough.

`Scripts/test-varia-ui.py` boots actual native frames and checks Vanilla pixels/RAM with options off versus on; region collection counts; maximum/current ammo; manual surplus preservation, cancellation and re-selection; automatic reserve surplus and invincibility; actual reserve-item PLM pickup with the option off/on; timed objective notifications, and the queue for all four bosses followed by the all-objectives message. Ammo glyph pixels match between 4:3 and widescreen. The reserve pickup fixture replaces only Morph's placement in memory for that test; it is not represented as a newly verified generated seed.

The separate existing native tracker suite checks pause/minimap colors, stale contexts, simulation parity and Vanilla opt-in against the new library. The actual Unreal Game binary also passes its headless asynchronous tracker controller self-test across Vanilla and two randomized contexts ([log](unreal-selftest.log)). Native, Unreal Game and Unreal Editor builds are recorded separately. Screenshots show the native renderer; final Vulkan composition and interactive menu readability require the user's next requested play session.

Evidence files and captures in this directory describe the exact checks actually run.

[Native verification](native-verification.json) · [Tracker regression](tracker-regression.json) · [Builds and hashes](build-evidence.json) · [Asset source hashes](asset-sources.json)

![Native VARIA HUD](hud-full-reserves.png)

![Partial energy and full reserves](hud-half-tanks.png)

![Three-digit maximum and current ammo](hud-three-digit-ammo.png)
