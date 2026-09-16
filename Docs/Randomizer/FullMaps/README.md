# Full original maps for randomized saves

Randomized games automatically display the complete ROM-authored maps, including
rooms and passages omitted by map stations. This applies to existing generated
seeds as well as new games; no regeneration or save conversion is needed.
Vanilla games retain their original discovery behavior.

The display change covers the native pause map, load-game room map, original
minimap, expanded widescreen minimap and full-map image API. Scroll limits use
the complete map footprint. Original map tiles, colors, icons and elevator labels
are retained. Ceres has no elevator-label table and is excluded from that overlay.
The original restriction on pausing during Ceres gameplay remains in place.

Explored rooms still receive their original explored color. Revealing the map
does not mark rooms as visited, activate map/save stations, open doors, collect
items or change collision. SRAM exploration and tracker snapshots remain real
progress records. The display follows the currently selected slot's seed mode,
so a Vanilla slot in the same bank keeps its own discovery rules.

## Authored map coverage

A case is an original 8×8 map tile, not a room or an individual destructible block.

| Area | Authored cases | Cases normally omitted by map stations |
| --- | ---: | ---: |
| Crateria | 233 | 50 |
| Brinstar | 254 | 86 |
| Norfair | 361 | 131 |
| Wrecked Ship | 69 | 24 |
| Maridia | 278 | 53 |
| Tourian | 79 | 34 |
| Ceres | 15 | 0 |

The reveal mask is derived from each original 64×32 tilemap at ROM table
`82:964A`, excluding blank tile `0x1F`. It is held outside gameplay RAM. Vanilla
uses the original station masks at `82:9717`. No ROM bytes are changed for this
feature, and no new map geometry is invented.

![Complete original Brinstar map](1-full.png)
![Original widescreen pause displaying Maridia](4-pause.png)

## Validation

`Scripts/test-randomizer-full-map.py` runs only on gaming-pc inside a marked test
folder. It loads native save data and the native Landing Site loader, then uses
controlled map-area/discovery fixtures to exercise all seven authored maps.
Ceres's renderer is explicitly entered for this fixture; this is not a gameplay
claim about pausing in Ceres or travelling through every region.

Two seeds are checked against all 2,048 original map entries per area in the
actual pause VRAM, including the station-omitted tiles. The native minimap is
checked against the same original tile source. Real exploration remains sparse
and map-station flags remain zero. Vanilla is compared against the previous
native binary using full gameplay RAM, native pixels, map-image output and HUD
data. Native and widescreen captures are inspected visually.

See [verification.json](verification.json), [mixed-slot regression checks](slot-verification.json) and [build-evidence.txt](build-evidence.txt).
Tests use isolated save copies and leave the source ROM and installed user saves
untouched. No interactive game window is launched.
