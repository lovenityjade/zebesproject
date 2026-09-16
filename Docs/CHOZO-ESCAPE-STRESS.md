# Chozo escape warning and recovery

At the required quota, the ordinary Chozo Tablet pickup message completes first.
A second native item dialog then displays:

> SOMETHING'S WRONG,
> GET TO THE SHIP !
> STRESS GIVES
> DETERMINATION !

The original 8-pixel glyphs and four-row message body are used, including the
original punctuation tiles. Space Jump is acquired and equipped at this point;
its original location is not marked collected. Neither the timer, escape event,
music change nor environmental explosions start until this warning is dismissed.

During the escape, actual received damage is doubled after suit mitigation.
Both direct damage and fractional environmental damage are covered. Frozen
gameplay and the original damage sentinel retain their behavior. The modifier
ends when Samus boards the ship. Native timer durations remain 3/5/6/7/10 minutes.

Original Zebes escape room states introduce sealed gray doors in Crateria.
Tablet escape retains the ordinary room states for event 14, keeping those
routes available without removing boss locks or seed-specific colored doors.
The original escape event remains set for the timer and ending. Ordinary
Mother Brain escape retains its original room-state behavior. VARIA's ban on
using save stations during escape is waived only for active tablet escape.
The optional refill setting remains authoritative.

## Existing saves

The escape sidecar retains its size and checksum format. Checkpoint status 2
means the warning has already been acknowledged; legacy status 1 resumes its
remaining timer after showing the warning once. Both paths ensure Space Jump is
present. A new save writes status 2, preventing repeated warnings on reload.
Invalid checkpoints cannot provide an invalid BCD timer or another seed's time.

The user's September 16 backup is stored privately at
`.tmp/escape-repair-backup-20260916-053239/profile`. Its bank contains four of
five required tablets and no active escape checkpoint. The original SRAM and
seed are preserved: there is no missing escape timer to reconstruct. On the
next tablet, the corrected game grants Space Jump and starts the new sequence.
A private gaming-pc replay verified that exact seed and saved inventory.

## Weapon selection

LT / Q selects the previous available HUD item; RT / E selects the next.
Both bindings are configurable. Original Select and Cancel still work, and
the original selection routines own ammo/equipment availability and palettes.
One press advances once; holding does not repeat, simultaneous triggers cancel
each other, and native pause/dialog input cannot queue a delayed change.
Existing custom bindings take precedence over conflicting new defaults.

## Targeted validation on gaming-pc

- `test-chozo-final-pickup.py`: physical pickup through the original PLM,
  tablet message, warning, then escape; no timer/event before acknowledgement.
- `test-chozo-escape-lifecycle.py`: all five durations, saved remaining time,
  native timeout. `test-chozo-timed-ending.py`: ship departure, ending, credits
  and finished chronological route.
- `test-chozo-stress-and-input.py`: direct and environmental damage, suits,
  sentinel/freeze, five native escape-room selectors, bidirectional selection,
  held/simultaneous input, original Select/Cancel, actual save-station prompt,
  save/refill, and migration of a legacy checkpoint without resetting time.
- `test-chozo-player-save.py`: original seed 1008746831, copied save at 4/5,
  modeled next collection bit, 5/5 warning/gift/timer, preserved other inventory,
  and unchanged backup. This is not a physical playthrough of that seed.

All use isolated SMTests saves. No local compilation or game launch is required.
These checks do not claim exhaustive playthroughs of every escape route.

September 16 delivery: Native, Game and Editor builds completed on gaming-pc.
The Game and Editor async tracker tests passed; the early Editor fixture used
`PYTHONHOME` pointing to that engine's bundled Python, before normal plugin
initialization. Validated artifacts were transferred and installed locally
without launching the game. Previous binaries are retained privately under
`.tmp/binaries-before-chozo-stress-20260916`; logs are under
`.tmp/chozo-validation-20260916`.

Installed native SHA-256:
`0d3de6362c4c79ebfac88ae396d5425196b5de345db4600a233171cdbf629b96`.
