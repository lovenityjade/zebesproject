# Native area selection from pause

Implemented 2026-09-15. The original **PLANET ZEBES / Area Select** screen from
saved-game loading is now available from the native pause map. It uses the
original six-region tiles, background grid, title and region spritemaps, active
and inactive palettes, selection sounds and display order. It is part of the
native C core, with no ImGui popup or external map window.

## Controls

| Screen | Native button | Action |
| --- | --- | --- |
| Pause map | Select | Open the original region overview |
| Region overview | D-pad / Select | Choose another available region |
| Region overview | A / Start | View its original native map |
| Region overview | B | Return to Samus's current map and original scroll |
| Viewed map | D-pad | Scroll the native map |
| Viewed map | B / Select | Return to the region overview |
| Viewed map | R | Open native Samus equipment |
| Viewed map | Start | Resume gameplay in the actual room |

With the default port bindings: **Enter** opens pause, **Right Shift** is
Select, **Z** is native A, and **X** is native B. Controller bindings and
remapped port keys still work through the existing input system. Holding Start
to select a region cannot also unpause: release it before pressing again.

## Check counts and visibility

Each region shows **available OF total** below its original name; the selected
region's count is green. For example, `02 OF 31` means two currently accessible,
uncollected checks among Brinstar's 31 item locations. The same count appears
at the bottom of a remotely viewed map. Counts use individual checks, so two
locations sharing a map square are counted separately.

Only green tracker results count as available: advanced routes / uncertain
returns (yellow), blocked (red), inspect-only (blue), and collected checks are
excluded. The denominator is the region's total item-location count. Tourian
has no item checks and correctly shows `00 OF 00`; boss markers remain native.
While results are pending or stale, **WAIT** replaces the number. Nothing
reports a false zero before the seed's effective logic has finished.

Randomized games can select all six regions. Vanilla defaults to the current
region plus regions with actual exploration or a map station, with no tracker
counts. Enabling **Trackers in Vanilla** and **Map tracker** exposes all six
authored maps and the same counts. Turning this off while viewing an unknown
region returns to the actual current map. The overview also updates its
selection if the selected region becomes unavailable.

## Native integration and preservation

`Native/sm_map_browser.c` calls the original `MapVramForMenu`,
`LoadInitialMenuTiles`, `LoadMenuPalettes`, region-palette routines,
`FileSelectMap_2_LoadAreaSelectForegroundTilemap`,
`LoadAreaSelectBackgroundTilemap`, `SwitchActiveFileSelectMapArea` and
`DrawAreaSelectMapLabels`. It never enters `SelectFileSelectMapArea` or
`LoadFromLoadStation`, so browsing cannot load a save station or teleport.

The overview temporarily borrows paused menu RAM, PPU and DMA state. On return
it restores the pause context, keeping live input and elapsed clocks. The
pause HUD scanline IRQ is disabled only for the overview and restored on exit.
Remote maps use a scoped area/exploration context around the original pause
map load, bounds, scrolling, labels, icons and elevator markers. The actual
room, area and exploration are restored before the simulation step returns.
Samus's position indicator is drawn only in her actual region.

`sm_tracker_render` selects marker coordinates from the viewed region while
its inventory snapshot and asynchronous logic remain bound to the actual
slot/session/seed. `sm_tracker_area_count` shares the accepted live states and
stale-result checks; it does not query spoiler placements or infer item grants.
Original pause-font letters and HUD digits provide the new counters and hints,
with a one-pixel black outline for contrast. There are no generated assets.

Upstream checkouts remain untouched; bank changes are generated through
`Native/prepare_overlays.py`. The existing native pause and equipment UI remain
the normal path when Select has not been pressed.

## Verification

[Native verification](native-verification.json), from
`Scripts/test-map-browser-native.py`, runs on **gaming-pc** in its marked
isolated test directory. It covers all six regions, real native navigation,
scrolling, per-region counts against live logic, 89 visible marker color checks,
current-map return, native equipment transitions and resume. It verifies exact
preservation of slot, room, area, player map position, acquired/equipped items,
beams, collection/boss/event/door flags, current/saved exploration, map stations
and all 8192 SRAM bytes while browsing. Pending/stale results, immediate
collection-count updates, Vanilla visibility, and opting out on a remote map
or on the overview are exercised. Emulated CPU instruction count stays zero.

The captures are rendered by the actual native PPU and widescreen compositor.
They verify native pixels; no new interactive local game session or Unreal
Vulkan capture is claimed by this test.

The existing [native pause regression](pause-regression.json) also passes:
720 frames compared between native-width and widescreen runs, covering map
scroll, equipment, return and resume, with exact original panel/map pixels.
[Build evidence](build-evidence.json) records source/library hashes and verifies
that the user's repaired randomized save remains unchanged.

![Original region selection with live counters](overview.png)

![Maridia viewed from another region](maridia.png)

![Vanilla region visibility](vanilla.png)

![Pending logic explicitly displayed](pending.png)
