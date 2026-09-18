# Seed-aware native map, saved-station travel and refill defaults

Implemented and automatically verified on gaming-pc, September 17, 2026.
The arrangement is ready for human review; no release was published.

## Requested behavior

The user selected **rearrange zones according to the seed**, rather than keeping
the vanilla map and merely changing destination labels. Checks, boss markers,
objectives, Samus and the local minimap must agree with the actual seed topology.

The native atlas splits the authored map at the 32 area and 8 boss access points.
Its 25 pieces retain original room tiles and orientation. Fixed connections stay
fixed; seed connections determine a deterministic arrangement. Dotted links
represent transitions, not traversable room tiles. The original pause frame,
ROM map graphics, fonts, station sprites and player cursor are retained.

- D-pad: pan; Y: cycle native-pixel zoom levels; A: recenter on Samus.
- Select: native area selector; selecting an area focuses its map piece.
- Start, equipment and objective controls keep their native roles.
- Ordinary item-only seeds and Vanilla retain the original regional maps.
- Minimizer filters missing tiles and checks before laying out the retained map.
- The local minimap hides old adjacent pieces disconnected by shuffling.

Presentation coordinates never replace a location ID, room address, inventory
bit, save-station bit or solver coordinate. Item squares and boss diamonds use
the existing effective-seed evaluator and PopTracker state colors. Stale results
remain pending instead of becoming green. Existing slot/fingerprint guards still
reject a response belonging to a different seed. No PopTracker pack is required.

## Save stations

New installations default **Refill energy and ammo when saving** to on. Existing
explicit preferences are preserved. Confirming a station save restores energy,
reserves and all three ammo types before writing SRAM; cancelling does not.
Both speedrun categories suppress this comfort setting. Seed-required refills
remain enabled independently of the local preference.

**Travel between saved stations** is an optional Quality of Life preference,
off by default. It applies only to generated Randomized slots. The native load
screen selects a region, then L/R selects an unlocked station, A/Start loads,
and B returns. Slot A/B/C never share unlocks. Map reveal does not unlock travel;
elevator/debug entries are not destinations. Loading a destination does not
rewrite the saved checkpoint. Travel is disabled during an escape or speedrun.

## Recorded follow-up: all doors

The user requested an option that can randomize **every door**, beyond the
current supported subsets. This is recorded, not implemented by the atlas.
Current connection catalogs cover area boundaries and major-boss entrances;
colored-door randomization has its own reviewed door list and exclusions.
A future all-door option must explicitly cover internal room connections and
applicable locks, and include traversal/return-path safety, boss scripts, save
access, solver topology, tracker projection and seed-format compatibility.
No unsupported all-door setting is exposed as working.

## Verification

Use `Scripts/test-seed-atlas.py` only on gaming-pc in an isolated test directory.
Results are recorded in [verification.json](verification.json):

- Three full/light/area-plus-boss seeds: all 100 check identities projected,
  1,200 exact check-color samples and 120 boss-color samples verified. Different
  connection plans produce different layouts. Native area selection, including
  holding Start to confirm, preserves gameplay state and SRAM.
- Six saved solver progression logs: 600 next-check reachability assertions with
  the effective settings/topology; 6,600 boss-state records checked for valid
  states. These are solver fixtures, not six physical gameplay walkthroughs.
- Three Minimizer seeds: 42 / 64 / 100 retained checks and 447 / 777 / 1,165
  retained authored tiles. Membership follows the native minimizer and projected
  cells do not overlap.
- Travel: actual native area/station menus, L/R cycling, then a gameplay arrival
  in Brinstar room `A201`. No checkpoint rewrite; Vanilla/pending slots, full-map
  knowledge alone and escape state cannot grant travel.
- Refill: six real station-interaction cases (Vanilla/Randomized, enabled/disabled,
  accept/cancel), including save reloads. Native default verified on.
- Native C and Unreal Editor Development builds passed remotely. Unreal/Vulkan
  map, equipment and unpause navigation passed (`SM_NATIVE_PAUSE_PASS`), with
  zero emulated CPU instructions. The 1280 x 720 map capture was visually reviewed.
- Private images and full logs are in `Build/SeedAtlas/`; source ROM, donor save
  and personal saves were not changed. Nothing was compiled or launched locally.

The existing Unreal fixture needed to navigate down to Start Game after the
mode-menu redesign; its automatic startup now does so. Its timed button sequence
requires `-ExecCmds=t.MaxFPS 60`, as in the existing pause-test runner. A stale
remote UBT directory cache was refreshed before building newly synced sources.

Remaining validation is player feedback on map arrangement/readability and longer
randomized runs. The all-door option above remains a recorded future addition.
