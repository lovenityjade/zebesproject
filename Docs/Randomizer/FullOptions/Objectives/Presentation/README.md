# Native VARIA objective page and HUD notifications

The original pause menu now opens the native objective page with L from the map. R returns to the map, Start resumes gameplay directly, and Select retains the existing area browser. Equipment navigation remains the original map/R, equipment/L path. This checkpoint does not enable public nondefault objective generation: objective map icons and remaining dependencies still need integration, followed by the complete outstanding world/Scavenger/Tourian/escape/race/patch scope.

## Original assets and native state

`Scripts/build-objective-pause.py` extracts the authored VARIA objective frame, font, palettes and goal text from the pinned source. Only data are imported; executable IPS instructions are never installed. The native footer retains Super Metroid's L/R, EXIT and START graphics. Its conflicting graphic indices are relocated while the objective page is active, then restored. The OBJ label also has separate slots so it cannot overwrite START or the equipment wireframe. Native shoulder animations use valid original table entries for the available controls.

`sm_objective_pause` extends the native pause state machine, including original fades and audio feedback. It reads the applied objective snapshot, draws completion ticks and in-progress counters, hides unrevealed goals and scrolls a list of up to eighteen entries. Regional-clear counters require the original regional start event; enemy-family counters begin after the first kill. Source percentage arithmetic remains in the native evaluator. Goal ranks follow the effective ordered seed contract. The original `Kill` synonym is fixed for deterministic text.

The 4:3 view uses original 8×8 tiles. The widescreen view extends the original frame and reflows those same glyphs without stretching them or splitting words. Long progress suffixes wrap onto the spare row in 4:3. The Tourian label is Vanilla because the current objective ABI explicitly permits only ordinary Tourian; alternate Tourian behavior remains guarded. No replacement popup, generated artwork or external tracker pack is required.

Palette, graphics, footer and map-scroll restoration are part of the native transitions. Start takes priority over a simultaneous shoulder press. Vanilla and historical plans without an applied objective descriptor keep their existing pause behavior. SRAM layouts and seed-plan layouts are unchanged.

## Persistent HUD notifications

For plans with native objectives, the HUD follows VARIA's checker index and its original reset/all-complete behavior. Individual messages use the effective list number (`Obj OK! 1` through `Obj OK! 18`); the required quota uses `Objs OK!`. Hidden goals are not revealed by these messages.

Notifications last 300 native frames. Expiry or opening pause marks the source notification event: custom event 128 for the required quota and 163 + 2 × index for an individual objective. These flags live in the existing versioned ZOE1 save extension and persist independently in A/B/C. Disabling the HUD does not acknowledge messages the player did not see. Old plans retain their historical four-boss notifications, and Chozo Relic Hunt retains its separate notifications.

## Validation

All runtime work used the isolated directory `/tmp/sm-native-generation-20260914` on gaming-pc. No local game was launched and the local packaged runtime was not replaced.

- [Native pause results](native/verification.json): hidden/revealed pages, source progress rules, eighteen-entry scroll limits, original equipment navigation, Select area browser, exact map-graphics restoration, scroll preservation, direct unpause and simultaneous Start/R. There are 320 original footer tile/graphic checks.
- [Native HUD results](hud/verification.json): all eighteen displayed ranks, exact countdown boundary, hidden-goal behavior, quota versus all-complete, disabled-HUD behavior, three original checksummed SRAM slots and six reloads, real frame entry and Vanilla gating. These are controlled event fixtures, not eighteen physical playthroughs.
- [Unreal results](render-results.json): the two already generated Logic seeds 15092901 and 15092902 pass the actual asynchronous tracker request, pause/objective/equipment navigation and direct resume under Vulkan. One has hidden goals and shuffled areas; the other has visible regional/animals/robots goals. Six screenshots per case record the original menu transitions. This increment reuses those verified generated plans; it does not claim two newly generated seeds or complete physical seed playthroughs.
- [Regression results](regressions.json): all 58 native conditions, native-to-embedded-oracle stale-state checks, 3,394 original map sprite comparisons, and the current historical HUD/reserve suite pass against the final candidate. The latter includes original four-boss messages, manual/automatic reserve handling, actual reserve pickups and Vanilla pixel/state parity.

The initial remote copy of the historical HUD test was older than the repository copy and failed its unpause assertion. After synchronizing the current full script, it passed on both the previous Logic binary and the Presentation candidate. The diagnostic failure is retained in `test-varia-ui.py.log`; final candidate evidence is [legacy-candidate/verification.json](legacy-candidate/verification.json). The baseline diagnostic's original report writer hashed the staged runtime instead of its overridden loaded library; use the recorded comparison metadata for binary identity.

Native, Unreal Game and Unreal Editor builds pass. [Build hashes](build-hashes.json) identify the candidate and relevant sources. [Preservation](preservation.json) verifies all frozen Save Refill hashes and all three pristine pinned upstream repositories. Only the isolated gaming-pc runtime was staged.

## Visual inspection

The following final captures were inspected: [hidden objectives in Unreal](01-objective-pause-200.png), [visible objectives in Unreal](02-objective-pause-200.png), [native completion/progress](native/revealed-progress-wide.png), [restored equipment](native/equipment-after-objectives.png), and [two-digit HUD objective](hud/objective-18.png). Original L/R glyphs, EXIT/START, frame tiles and text are intact. Earlier footer conflicts and the additional-page animation-table overrun were fixed before these captures.

## Next integration

Implement original objective-number map icons and suppress the replaced original boss boxes according to source rules. Capture/derive their exact effective goal membership, sub-events and positions, preserving hidden visibility and area/boss topology. Then implement Bomb Torizo sleep and objective sound priority dependencies before opening nondefault objective generation to the public configurator. Continue the unchanged full scope: ordered Scavenger, modified/mirror worlds and Minimizer, alternate Tourian and escape/race, remaining relevant patches, and menu-to-generation/solver/tracker/save/render validation for those modes.

The pinned source draws objective icons from `map/PauseScreenRoutines.asm:draw_objective_icons`, using the objective rank and optional sub-event. Its hidden flag checks the reveal event; the original boss boxes are skipped. Bomb Torizo sleep is the `option_BT_sleep_mask` branch in `bomb_torizo.asm:btcheck`. These are implementation pointers, not claims that those dependencies are already complete.
