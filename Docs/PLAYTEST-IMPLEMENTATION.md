# Playtest implementation — September 16, 2026

The user authorized the remaining playtest backlog after completion of Game Over.
Preserve user saves and the existing worktree. Test game runtime on gaming-pc in
its marked isolated test directory. Do not launch the user's local game.

## Confirmed decisions

- Chozo escape begins at the required quota, not all placed tablets.
- Native escape timer choices: 3, 5, 6, 7, 10 minutes. Default: 5 minutes.
- Escape: red alert, environmental explosions, Escape music; boarding the ship
  stops the timer and continues ship departure, ending, then credits.
- NG+ imports equipment and energy/ammo capacities from the completed Vanilla
  game. Enemy health ×2, inflicted damage ×1.5, unchanged animation speed.
- Speedrun ranking uses native in-game time. Display it under Energy in the
  native font and size. Real elapsed time may also be shown.
- Vanilla and NG+ have separate No QoL / QoL categories. Glitches allowed.
  Only Wall Jump and Space Jump assists qualify in QoL; no gameplay time savers.
- Online: prepare database and reception service on nexus-vps. Game and eventual
  website will consume it later. Discord sign-in/public leaderboard are deferred.

## Work order and acceptance

1. [x] Fix sprite vertical wrap, transition HUD instability and eye lighting.
   Capture boundary/transition cases; compare simulation with presentation off.
2. [x] Native map screen cleanup; save-station lighting/electricity.
   Include the earlier Speed Booster electricity / destruction-flash note.
3. [x] Achievement notifications, short native sound and achievement screen.
4. [x] Chozo quota escape and configuration, reload/death/ship ending tests.
5. [x] NG+ completion snapshot, isolated new save, scaling and repeat-load tests.
6. [x] Speedrun categories, eligibility, native timer and completed-run records.
7. [x] Randomizer chronological route recording and native-map playback.
8. [x] Nexus database/receiver, schema validation and isolated request tests.
9. [x] Final builds, targeted integration/render tests, documentation and review.

No item is marked complete until its corresponding implementation and validation
have passed. Do not equate compiling with gameplay or online verification.

## Validation checkpoint

Implementation and targeted verification completed. See [PLAYTEST-FEATURES.md](PLAYTEST-FEATURES.md)
for behavior, controls, sidecars, tests and the limits of coverage. Native/Game/Editor
builds passed; runtime and rendered checks used isolated gaming-pc fixtures.
The user's local game was not launched. The upstream native-core checkout is clean.

Additional user report: save-station refill was disabled both in the local
preference and the active seed. Enabled the personal preference for the next
launch, preserved the seed, and verified six real station cases plus both
speedrun exclusions. No game save was changed for this configuration correction.

Nexus receiver is active, enabled and bound to loopback port 28740. Live test
records were removed by their own UUID; public rankings and Discord remain deferred.

Follow-up completion pass: route copies now inherit their source history and
archive the destination; clear/reuse cannot merge old histories. Enemy respawns
reset NG+ fractional damage. The native HUD copy also excludes vertically wrapped
sprites. Unreal recap controls and automatic opening were rendered/verified, and
the save-station effect was verified from a real native confirmation event.
Fresh Start also resets run identity/eligibility and reapplies NG+ gear while
Load preserves the existing run. Final Game/Editor builds passed, and the native
library is installed locally. Speed Booster charging is verified natively;
its engine light/particles were inspected with the documented charged-state
render fixture at Landing Site, outside Norfair's rising-lava overlay.

## New playtest reports — gameplay follow-up

Priority confirmed September 16: finish the current gameplay fixes below before
the UI reorganization. The UI request is deferred, not cancelled.

- [x] At the required Chozo quota, show an original item-style warning before
  starting the timer, music or explosions: "Something's wrong, get to the ship !"
  and "Stress gives determination !". Grant/equip Space Jump and apply double
  damage during the escape. Preserve remaining time when migrating an existing
  escape. Validate the user's current save after backup: it is at 4/5, so keep
  its progress intact and apply the corrected sequence at the next tablet.

- [x] Chozo escape locks doors leading to save stations after the required
  tablet quota triggers destruction. Preserve normal room routes in tablet
  escape and allow stations; ordinary escape and boss/seed locks stay intact.
- [x] Bidirectional HUD item selection: LT selects the previous available item;
  RT selects the next. Replace reliance on Select's forward-only cycling.
  Check existing controller bindings and handle unavailable equipment correctly.
- [x] Boss accessibility diamonds on the native map/minimap, including Spore
  Spawn. Same oracle and effective seed rules as item checks; no item behind a
  boss is assumed collected. See [Tracker/BOSS-MARKERS.md](Tracker/BOSS-MARKERS.md).
- [x] Spore Spawn spores and light fog, rendered and inspected in Unreal on
  gaming-pc. See [SPORE-SPAWN-ATMOSPHERE.md](SPORE-SPAWN-ATMOSPHERE.md).

Behavior, checkpoint migration and targeted test evidence:
[CHOZO-ESCAPE-STRESS.md](CHOZO-ESCAPE-STRESS.md).

Current workstation constraint: all further compilation, asset preparation,
and tests must run on gaming-pc. Keep the user's local game available for play;
do not run resource-intensive build or validation tasks on the local machine.

## Deferred until the current fixes are complete — UI and start flow

- [x] Reorganize the English UI into clear, readable categories for all players;
  distinguish everyday settings from advanced settings without removing options.
- [x] Start directly on the native game title, without opening the UI menu.
- [x] Choose Vanilla or Randomized in the native Start Game menu for the selected
  A/B/C slot. Prepare settings in the UI, then use them for native generation.
  Preserve independent modes, seeds and saves per slot, and generation gating.

User explicitly deferred this work on September 16: "Tu le fera après ce que
tu faisais, note le au pire."

UI follow-up completed September 16. See [MENU-START-FLOW.md](MENU-START-FLOW.md).
Game and Editor builds passed on gaming-pc. The actual native menu plus Unreal
profile callbacks passed in both hosts (Editor used NullRHI); A/B/C regression
passed. Fifteen Game-host captures cover common/advanced settings, search and
640x480 fallback navigation. The editor graphics attempt timed out while building
its fresh shader cache, so it is not counted as rendered evidence. Local saves
were not used by these fixtures; no local build or game launch was performed.
