# Temporary Maridia X-tile replacement

The original room data uses metatile `3FF` (solid block `83FF`) as a visible X
marker in unused screens beyond the native camera limits. Widescreen exposes
parts of those screens. An audit of all Maridia room states found 6,656 such
foreground cells, across nine rooms.

The replacement is a presentation fallback in `Native/sm_wide.inc`. It affects
only the reconstructed spans (widescreen margins and the newly exposed top band),
not the native PPU or level data. When an authored decoration causes the center
to be reconstructed as well, the same fallback applies there.

| Rooms | Tileset | Original replacement material |
| --- | --- | --- |
| East Tunnel, Main Street, Mama Turtle Room, Mt. Everest, Crab Shaft | 11 | Dark west-Maridia rock interior, tile `11D` |
| Northwest Maridia Bug Room, Pseudo Plasma Spark Room, Plasma Spark Room | 11 | Continuous sand, tile `210` |
| Halfie Climb Room | 12 | Purple masonry, alternating original tiles `1E3` / `1E1` |

Only exact `83FF` blocks in these rooms and tilesets are eligible. Background
layers, doors, slopes, breakable blocks, enemy art and existing decorative motifs
are not replaced. No new artwork is shipped: rendering uses the active ROM's
original tile graphics, palette and animations. No collision/BTS, room data,
simulation state, RNG, SRAM or ROM is changed.

Author-painted cells always take precedence, including a deliberate choice of
the original X tile. The temporary defaults create no JSON files and do not
modify existing room decoration files. The external editor continues to display
the original ROM and the user's authored paint; these defaults are runtime only.
Once the rooms have authored replacements, the per-room defaults can be removed.

Validation uses `Scripts/test-maridia-fill.py` and the existing isolated native
boot helper on gaming-pc. Each room starts in a fresh process to avoid unrelated
music-driver behavior from rapid debug teleports between rooms. The test captures
native room loads facing unused screens, using copied SRAM only. Before/after
images and comparison reports are kept privately under `Build/MaridiaAudit/`.

The native build and nine room captures passed. Eight views expose markers and
show their replacement; the selected Northwest Maridia Bug Room view exposes no
X cells. The ordinary-room control is unchanged. Every compared frame has
identical simulation RAM, original PPU output and central gameplay pixels.
A deliberate authored X in Halfie Climb renders above the temporary fallback,
confirming editor priority without modifying its underlying solid block.
The captures were visually inspected; this is not a manual Unreal traversal.
No published release has been replaced.
