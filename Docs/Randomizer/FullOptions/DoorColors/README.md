# Native randomized door colors — 2026-09-15

The existing World & escape settings now connect `doorsColorsRando` and `allowGreyDoors` to native generation and independent A/B/C plans. The generator captures VARIA's actual `writeDoorsColor` output and verifies every written byte against a versioned 58-location table. The fresh solver and live tracker restore those exact door requirements; colors are not reconstructed from a preset later.

## Native behavior

`sm_doors.c` installs 12,255 bytes of unchanged PLM lists, draw data and compressed graphics from the pinned beam-door/red-door patches. It excludes 171 bytes of injected 65816 routines and implements their five pre-instructions in C. Wave, Ice, Spazer and Plasma doors require their original beam bits; owning both Spazer and Plasma permits either beam as VARIA specifies. Bomb hits are rejected. Red doors take exactly one ordinary missile and reject supers. Random grey doors use argument `$90xx` and remain sealed during escape/normal room-clear conditions.

The original blue-door opening/closing animation, shot sound, opened-door bits and native save persistence are retained. Four beam colors in four directions have matching blue/color blink indicators. X-ray excludes these high-address door/indicator PLMs from the old item range. Configuring or activating beam indicators without a colored-door plan is rejected.

The new-file path sets the forced-blue and refill/save door bits from the slot's effective table. These initial flags do not reset progress on a later save reload. Native data restoration is separate from the ordered world catalog: restore color-owned bytes first, apply starts/layout, then apply the generated door colors. Pending configuration does not change the applied table. Vanilla restores original PLMs, arguments, graphics and red-door behavior. Old plans without `native-door-colors-v1` retain their previous behavior.

`native_door_colors.json` binds color IDs, location identities, opening-bit IDs, direction, source patch digests and data spans. `SMNativeDoorCatalog.inl` is generated from it for Unreal validation. The UI validates all 58 native/tracker requirements and the versioned capability before publication. Generation remains in the native new-file menu. `door_color_catalog_history/vanilla-v1.json` freezes the published identities.

## Verification on gaming-pc

All runtime tests use the isolated `/tmp/sm-native-generation-20260914` copy. No local launch, no user-save editing, and no upstream checkout modifications.

- `Proofs/seeds/verification.json`: four real seeds 15092500–15092503, each with all 100 checks reachable and completion verified by the fresh solver. Two use No Advanced Techs, two allow grey doors, and the last combines shuffled boss connections. Every seed exercises beam indicators. Actual grey placements are present in the allowed-grey fixtures.
- `Proofs/tracker/verification.json`: all 400 next checks from the four progression logs agree with the embedded live tracker, including collected-check removal. The inventory/boss flags are replayed from solver logs; this does not claim physical traversal/combat.
- `Proofs/native/verification.json`: eight full-ROM color-plan comparisons and restore/invalid-plan rollback checks; 300 hit-policy cases; forced-blue/refill initial opened bits; zero emulated CPU opcodes. Code bytes are excluded from installation.
- `Proofs/plms/verification.json`: actual native setup/pre-instruction/draw/open/delete lifecycle for 16 beam doors and 16 indicators; all four red orientations reject a super and open on one missile; all four permanently grey orientations remain sealed with room/boss events set. Opening bits survive native SRAM save/load, X-ray is safe, and a missing beam-data dependency is rejected.
- `Proofs/render/verification.json`: actual room loading, graphics decompression and closed/open captures of all 16 beam-door variants. These are deliberately inserted visual fixtures in room `$948C`, with full equipment restored between cases. The first fixture run exhausted Samus's health; the failed frame/state assertion caught death rather than a door-opening defect. The opening animation retains its original 112-frame duration.
- `Proofs/bank-1/verification.json`, `bank-2/verification.json`: native new-file generation commit, initial SRAM, slot-specific PLM bytes, reload, later save, copy/clear, and failed-save rollback across all four plans.
- `Proofs/historical-bank.json`: old start plans still pass native save-bank switching/reload/copy/clear.
- `Proofs/world-transactions.json`: all 50 existing ordered world-data transaction scenarios still pass with the new restoration module.
- `Proofs/door-colors-profile.log`: 29 Unreal profile fixtures, including all four colored-door plans, publication/reload and malformed contract rejection.
- `Proofs/door-colors-unreal.log`, `door-unreal.png`: actual offscreen Vulkan render of seed 15092500. The original KihunterRight PLM in room `$948C` is Plasma `$F74B`; 14 generated indicators are configured; zero emulated CPU opcodes. This is an actual generated room PLM, not the inserted visual fixture. Screenshot inspected, along with representative native Wave and Ice captures.

Native, Unreal Game and Unreal Editor builds pass. `Proofs/build-hashes.json` records future-launch staging, intact Save Refill release hashes and pristine upstream revisions.

## Remaining full objective

This increment does not complete full VARIA parity. Area connections, mirror, Minimizer and its saves/doors, ordered Scavenger, general goals/Tourian/escape/race, remaining gameplay patches and their end-to-end validation remain required. VARIA's door/portal symbols and exploration behavior on the pause map/minimap also remain to be integrated; current item-check availability already uses the effective colors and boss graph. Save-station travel stays paused and the Speed Booster effect request remains queued.

Area/start follow-up: carry the complete upstream `getStartDoors`/`getBlueDoors` initial door set in a versioned plan for new non-color seeds too. This increment initializes those blue/refill bits for colored-door plans; historical non-color plans retain their existing flags. Area/Minimizer will also add conditional start doors and PLMs.
