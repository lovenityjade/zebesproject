# Boss Rush — minimum equipment research

Research date: 2026-09-18. The difficulty proposal below is now superseded by
[the agreed five-level direction and development status](IMPLEMENTATION.md).
Status: research and design proposals, not implemented
loadouts. Baseline: original Super Metroid combat rules. The four menu labels
already exist: Easy, Medium, Hard, Hardcore.

## What “minimum” means

Separate offensive capability, ammo quantity, survival, movement inside the
arena, and access to the room. The proposed VR transitions remove travel between
arenas; they do not automatically remove heat, water, activation scripts or
post-kill scripts.

There is no single smallest kit: Charge trades finite ammo for time; a suit
trades an equipment upgrade for lower energy requirements. A damage calculation
does not prove that a player can survive the complete encounter with that kit.
The tables below establish offensive lower bounds and explicit exceptions,
not verified 99-energy, no-hit clears in this port.

Counts mean successful damaging hits, excluding misses, blocked shots and
invulnerability. Ordinary missiles deal 100 damage; supers normally deal 300;
unupgraded charged Power Beam deals 60. Boss-specific multipliers override
these values. Charge has no ammunition cost and takes about one second to
prepare. [Charge mechanics](https://wiki.supermetroid.run/Charge_Beam),
[base Charge damage](https://wiki.supermetroid.run/Ridley),
[missile damage](https://wiki.supermetroid.run/Mother_Brain_Room/Low_Ammo).

## Core encounters

The working roster is ten encounters: five major bosses and five minibosses.
Mother Brain's phases are one encounter. Entries link to the speedrunning
community's documented fight mechanics and strategies.

| Encounter | HP | Minimal offensive route | Arena / practical qualification |
| --- | ---: | --- | --- |
| [Bomb Torizo](https://wiki.supermetroid.run/Bomb_Torizo) | 800 | **Base Power Beam**, no offensive upgrade. 40 normal hits at 20 damage; alternatively 8 missiles. | The ordinary encounter starts after acquiring Bombs; activation must be handled separately in Boss Rush. Eggs supply missiles. Bombs are not needed to damage him. |
| [Spore Spawn](https://wiki.supermetroid.run/Spore_Spawn) | 960 | **Charge**, or missiles/supers. Calculated: 8 plain charges because charged damage is doubled, 10 missiles, or 4 supers. | Spores can replenish missiles, so one 5-missile capacity pickup can sustain a longer fight. Capacity and total shots are different quantities. |
| [Kraid](https://wiki.supermetroid.run/Kraid) | 1,000 | **Charge**, or 10 damaging missiles / 4 damaging supers. Calculated: 17 plain charges. | Opening the eye and damaging the mouth are different actions. Without a quick kill, reaching the upper platforms normally needs Hi-Jump or wall jumps. Hi-Jump belongs in a no-advanced-tech baseline, not in the offensive minimum. |
| [Crocomire](https://wiki.supermetroid.run/Crocomire) | Positional | **Charge**, or missiles/supers, to push him into acid. | No ordinary HP kill budget. Fireballs can supply ammo. Power Bombs make him charge forward; they are not a substitute damage kit. |
| [Phantoon](https://wiki.supermetroid.run/Phantoon) | 2,500 | **Charge**, or missiles/supers. Calculated: 42 plain charges, 25 missiles, or 5 supers. | Flames provide refills. A nonlethal super causes an extended rage phase: fewer shots can mean a slower fight. |
| [Botwoon](https://wiki.supermetroid.run/Botwoon) | 3,000 | **Charge**, or 30 missiles / 10 supers. Calculated: 50 plain charges. | Only the head takes damage. Do not budget an in-fight refill. Contact exceeds the initial 99 energy in Power Suit. Morph helps avoidance; no 99-energy kit is certified here. |
| [Draygon](https://wiki.supermetroid.run/Draygon) | 6,000 | **Charge** damages the belly; alternatively 60 missiles / 20 supers, before turret costs. Calculated: 100 plain charges. | Charge alone is an offensive bound, not a validated complete kit. Morph enables reliable swoop avoidance; Gravity changes underwater movement. The electrical route requires Grapple **and** ammunition to expose a turret, plus survival energy. |
| [Golden Torizo](https://wiki.supermetroid.run/Golden_Torizo) | 13,500 | **Charge** is the straightforward unlimited-ammo route. Calculated: 225 plain charges. | Heated arena: include Varia for a baseline without heat runs. He dodges missiles and catches supers in certain states. A mathematical 23 damaging supers does not guarantee a 23-ammo clear. |
| [Ridley](https://wiki.supermetroid.run/Ridley) | 18,000 | **Charge**, or 180 missiles / 30 supers. Charge alone takes 300 hits. | Heated arena: include Varia for the baseline without heat runs. Morph is useful for avoidance. The bare offensive minimum creates an endurance fight, not a short time trial. |
| [Mother Brain](https://wiki.supermetroid.run/Mother_Brain) | 3,000 / 18,000 / 36,000 | **Missiles/supers for MB1**, then **Charge** or sufficient remaining ammo for MB2. Hyper Beam is supplied by the native sequence for MB3. | Requires a separate glass/ammo budget and Rainbow Beam survival check. See below. |

The shot counts marked “calculated” are arithmetic deductions from HP and
damage, not measured clear times or gameplay validation.

## Arena and resource constraints

- Both [Ridley's room](https://wiki.supermetroid.run/Ridley's_Room) and
  [Golden Torizo's room](https://wiki.supermetroid.run/Golden_Torizo's_Room)
  are heated. Varia is a proposed baseline safety requirement, not an absolute
  claim that suitless kills are impossible. Heat-run kits need their own energy,
  duration and recovery proof. Do not silently disable heat to claim a native
  minimum.
- In-fight refill sources exist for Bomb Torizo, Spore Spawn, Kraid, Crocomire,
  Phantoon, Draygon and Golden Torizo. A low-capacity kit relying on farming
  adds random drops and time; “five missiles available” is not “five shots to
  win.” Kraid can lose his usable ammo source through a particular projectile
  despawn, so a farming-only kit is not a robust default.
  [Projectile drop tables](https://wiki.supermetroid.run/Enemies),
  [Kraid's ammo-source caveat](https://wiki.supermetroid.run/Kraid).
- For Draygon's electrical route, a turret has 300 HP: **3 missiles or 1 super**
  can expose a connection. Grapple conduction also drains Samus's energy.
  Morph and Gravity are practical kit choices to evaluate, not prerequisites
  for the turret's damage formula. Do not advertise “Grapple only.”
  [Draygon's turrets and electricity](https://wiki.supermetroid.run/Draygon).
- Use native post-kill completion state before switching rooms. Botwoon's
  central pillar must finish breaking for his death to register.
  [Botwoon's room](https://wiki.supermetroid.run/Botwoon's_Room).

## Mother Brain: mandatory special handling

For a fight starting at the intact jar, **10 missiles + 10 supers** is a
convenient capacity-based kit, not the unique mathematical minimum. Documented
jar-plus-MB1 combinations also include 36 missiles alone or 16 supers alone;
these round to capacities of 40 or 20 with ordinary five-round expansions.
For the 10/10 kit, spend six missiles on the initial glass opening, then ten
supers on MB1. Charge cannot replace this ammunition. This budget excludes
Zebetites, eye doors and the Tourian approach. Those must be removed from the
encounter scope or budgeted separately.
[Low-ammo analysis and exact combinations](https://wiki.supermetroid.run/Mother_Brain_Room/Low_Ammo).

MB2 can then be fought with Charge. Under original Rainbow Beam rules, plan
**Varia + 3 Energy Tanks** (399 maximum energy), arriving at the beam with
**more than 300 energy**. Without Varia, the corresponding ordinary capacity
is **6 Energy Tanks** (699 maximum), with **more than 600 energy** remaining.
Gravity alone does not substitute for Varia in this particular drain. Merely
owning the tanks is insufficient if combat has depleted them.
[Rainbow Beam mechanics](https://wiki.supermetroid.run/Mother_Brain).

These are ordinary survival thresholds, excluding glitches and special
Reserve Tank strategies. The port's native
[`Samus_DamageDueToRainbowBeam`](../../native-core/src/sm_a9.c) checks the Varia
bit specifically. VARIA's `NerfedRainbowBeam` and `TourianSpeedup` change the
solver's requirements. Boss Rush must pin its own ruleset rather than inherit
randomizer options from another save.

## Additional encounters to decide explicitly

These are not silently included in the ten-encounter roster:

- **Ceres Ridley:** no extra item needed, but this is a scripted retreat rather
  than an ordinary kill. It ends when Samus falls below 30 energy or after
  100 hits. Including the default encounter would reward taking damage as
  fast as possible; an offensive-only challenge would be a new rule.
  [Ceres behavior](https://wiki.supermetroid.run/Ridley).
- **Mini-Kraid:** 400 HP, ordinary enemy classification. Base beam is an
  offensive option; missiles and supers have a doubled multiplier, giving
  2 missiles or 1 super as quick alternatives. It could serve as a bonus
  encounter rather than be confused with Kraid.
  [Mini-Kraid mechanics](https://wiki.supermetroid.run/Mini-Kraid).
- **Two Metal/Ninja Pirates:** optional elite-enemy encounter. Each has
  1,800 HP and takes doubled super damage, giving 3 successful supers per
  pirate. Vulnerability windows still matter; this is not a six-shot input
  script. Do not assume the base beam can replace ammunition.
  [Fighting Space Pirates](https://wiki.supermetroid.run/Space_Pirate_(fighting)).

## Cross-check against our embedded VARIA logic

Inspected the current checkout (HEAD `9ccc57b`, with existing working changes).
The functions in
[`Randomizer/upstream/logic/helpers.py`](../../Randomizer/upstream/logic/helpers.py)
are reachability/safety estimates, **not a table of original boss HP**:

- `enoughStuffCroc` uses a 5,000-damage proxy despite a positional kill.
- `enoughStuffBotwoon` uses 6,000 normally / 3,500 for a low-equipment case,
  despite 3,000 actual HP.
- `enoughStuffGT` uses 9,000 / 3,000 approximations and particular drop rules.
- `enoughStuffsRidley` uses a 19,000 budget, Morph or Screw Attack, and extra
  recovery checks when not heatproof.
- `enoughStuffsMotherbrain` insists on the 10/10 capacity pattern and invokes
  `mbEtankCheck`; that method recognizes the two altered Rainbow/Tourian rules.

Reuse these to understand assumptions, not to label every rejected loadout
physically impossible. Source-code inspection and web research do not establish
successful runs with the future Boss Rush spawns, effects and transitions.

## Implications for the four difficulties — proposal only

Keep the user's single-attempt, minimum-equipment time-trial concept. Define a
small combat kit per encounter, then vary survivability and ammo margin before
adding arbitrary boss HP or speed multipliers.

| Difficulty | Proposed direction, not approved numerical settings |
| --- | --- |
| Easy | Practical minimal kit, comfortable energy/ammo margin, movement tools where needed; no required advanced techniques. |
| Medium | Same recognizable combat methods, smaller reserves and less tolerance for missed shots. |
| Hard | Tighter resources and equipment; explicit advanced techniques only where intended. |
| Hardcore | Lowest **validated** viable kits and strongest execution demands; retain every compulsory survival/arena requirement. |

Do not make Hardcore automatically mean “99 energy everywhere”: Mother Brain
contradicts that under native rules. Do not make it automatically mean “Charge
only everywhere”: Ridley alone would require 300 successful charged hits.

Before implementation, decide loadout reset versus carryover between fights,
refill policy, exact roster/order, allowed techniques, arena activation and
completion states, timer boundaries, and leaderboard separation by ruleset and
difficulty. Keep a record of initial equipment, energy, ammo, active patches,
clear time and successful native completion for each validated kit. Execute
those future game checks on gaming-pc, not during this research step.

No game build, runtime test, release or Boss Rush gameplay integration was
performed for this research.
