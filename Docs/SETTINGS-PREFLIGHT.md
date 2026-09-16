# Randomizer settings preflight

The Randomizer menu checks drafts automatically after a short edit debounce.
Each issue names the settings involved and links to the relevant category.
Results belong to the exact current request; stale asynchronous results cannot
mark an edited draft compatible. Generation runs the same preflight again.
With the revised [start flow](MENU-START-FLOW.md), creation takes place in the
native A/B/C menu. The system UI prepares defaults or edits a pending slot;
returning to the game remains possible even when a draft needs correction.

`sm_randomizer_validate` uses the same isolated Python runtime/mutex as generation and tracking. `sm_preflight.py` reads the pinned VARIA settings, presets and boss logic without generating placements, accessing the ROM or changing a save. Seed 0 is a valid unresolved draft. It checks option validation and native restrictions, tablet/endgame combinations, explicit objective exclusions, and mandatory boss difficulty. Multiple independent tablet conflicts are reported together. Normalizations inherited from VARIA are displayed as notes; random choices are rechecked during generation.

The boss check uses an optimistic full inventory (all upgrades, 14 energy tanks, 4 reserves and ample ammunition). A fight that exceeds the difficulty cap even with that inventory cannot be repaired by item placement. Optional bosses in minimizer worlds are excluded from this check. This is a conservative impossibility check, not a proof that a particular item pool or route works. Successful preflight never replaces the independent post-generation solver verification of every retained check and the chosen ending.

Generation also runs preflight, covering pending slots created before this feature. An incompatible request fails without eight futile generation attempts. A later route failure includes reachable/retained counts, blocked locations and VARIA's own difficulty warning. The native menu directs the player to Randomizer options; the detailed status is scrollable.

## September 16 incident

Seed 752526497 combined Easy, no advanced techniques, fast/easier progression, 30 tablets / 5 required, and Ridley tolerance `I'm scared!`. Before replacing any ammo, the source solver rated Ridley Hard, preventing access to Energy Tank, Ridley under Easy. The previous generic generation error hid that explanation.

A separate tablet-placement defect could replace progression ammunition. Tablet placement v2 protects VARIA's progression locations before replacing surplus ammo. The final route is still independently solved; a protected list alone is not proof. Existing generated slots are not regenerated. New tablet placements can differ from older versions for the same seed.

## Validation

Run only in an isolated gaming-pc directory containing `ISOLATED_TEST_DIRECTORY`:

```
python3 Scripts/test-settings-preflight.py /path/to/isolated-root
python3 Scripts/test-relic-generation-regressions.py /path/to/isolated-root
```

The first covers unresolved seed, invalid number, boss tolerance/cap, multiple tablet conflicts, quota, conflicting/compatible objectives, unknown patches, random choices and call isolation. The second covers the rejected original settings, three Easy seeds with adjusted Ridley tolerance, the original profile at Hard, and an ordinary non-tablet seed. Successful cases prove 100 reachable checks, completion and protected progression ammo. Ordinary seed 1032809128 retains fingerprint `95a02a476fcad493a8b793a17eeaf20c002f3a78e6ad90dde646a823d3532b27`.

Game and editor builds passed. Rendered UI checked on gaming-pc at 1280x720: incompatible draft disables creation and shows both correction links; corrected draft enables creation. Artifacts: `.tmp/relic-generation-incident/` locally and `/tmp/sm-native-generation-20260914/relic-incident/` on gaming-pc. These are solver, service and rendered UI checks, not a full human playthrough.
