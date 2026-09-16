# Native area selection — implementation record

Status: implemented and native-tested, 2026-09-15. Work resumed after the Morph
pickup incident was corrected. See [controls, implementation and evidence](AreaSelection/README.md).
The notes below preserve the original investigation and user direction.

## Confirmed direction

Reuse the original **Area Select screen shown when loading an existing save**,
not a replacement popup or a redesigned region list. Make it accessible from
the native pause map to select another region's original map. Preserve the
original graphics, labels, palettes, selection highlight and native controls.

Show available checks / total checks for each region on the overview. Proposed
clarification: distinguish reachable uncollected checks from all remaining
checks; only green tracker results count as reachable. Pending or stale logic
must show an unknown value, never a fabricated zero. Use the active slot's
effective settings and native collection flags. Honor Vanilla tracker opt-in.

Browsing must not load a save station, move Samus, change the real area, grant
exploration, change collisions, or modify SRAM. Returning to the current map,
equipment or gameplay must restore the actual room and native map scroll.

## Investigated integration points

- `native-core/src/sm_81.c`: `FileSelectMap_6_AreaSelectMap`,
  `DrawAreaSelectMapLabels`, `SwitchActiveFileSelectMapArea`,
  `LoadInitialMenuTiles`, `MapVramForMenu`, `LoadMenuPalettes`.
- Original area display order: Crateria, Wrecked Ship, Tourian, Brinstar,
  Maridia, Norfair (native indices 0, 3, 5, 1, 4, 2).
- Original overview foreground: ROM `81:B71A`; region backgrounds: `81:BF1A`
  plus 2048 bytes per area. Menu tiles: `8E:8000`, palette: `8E:E400`.
  Native active/inactive region palettes: `81:A4E6` and `81:A40E`.
- `native-core/src/sm_82.c`: native pause map load, scrolling, icons and labels.
- `Native/sm_tracker.c`: 100 incorporated check coordinates, accepted live
  logic snapshot, stale-result rejection, native map/minimap rendering.
- `Native/sm_pause_wide.inc`: existing native pause frame extension.
- Make upstream adaptations through `Native/prepare_overlays.py`; leave the
  upstream checkouts pristine.

The original file-loading state machine cannot simply run from pause:
`SelectFileSelectMapArea` selects a load station, and
`FileSelectMap_9_InitRoomSelectMap` calls `LoadFromLoadStation`. Those side
effects must be bypassed for browsing. The implementation reuses native overview routines inside a saved pause
context and scopes remote map loads without changing the real world. An
offline ROM tile atlas was inspected at
`.tmp/map-browser/atlas.png`; later native tests ran in isolation on gaming-pc.

## Priority incident

User report: in the current randomized run, pickup sprite and message both
identified **Morph Ball**, but native equipment and embedded item tracker
showed **Spring Ball**. Progression became blocked after two checks.

The manifest and saved bank confirm seed **14092032**, slot A (0), profile
`01A0A4746B647C6FAA0A30CAAC21B373`. The original ROM's hidden Morph PLM grants
Spring Ball; VARIA's existing correction was missing in our native port. The
precise ceiling pickup was reproduced on gaming-pc. Correction, save recovery
and multi-seed verification are tracked in
[the incident report](../Randomizer/PickupFix/README.md). The original save and
seed have been preserved. Map UI implementation has now resumed and completed.
