# Boss accessibility markers

The map and HUD minimap now distinguish item checks (squares) from fights
(diamonds). A boss is green only when the current inventory and effective seed
rules permit reaching and defeating it, with a return within those rules after
victory. Red means blocked; yellow means an enabled route above the configured
difficulty or an uncertain return; defeated bosses become small gray outlines.
Bosses do not add to the region's item-check totals.

Covered encounters: Kraid, Phantoon, Draygon, Ridley, Mother Brain, Spore Spawn,
Crocomire, Botwoon, Golden Torizo, and Bomb Torizo. Their map coordinates come
from the original room headers, including the native +1 vertical map offset.
On Bomb Torizo's shared map cell, a smaller corner diamond retains the separate
item square. The player cursor remains visible.

The oracle uses the same graph, preset, techniques, combat thresholds, doors,
and native defeated flags as the item tracker. The return calculation may
hypothetically mark this boss defeated to account for its exit opening, but
restores the oracle inventory immediately. It never borrows the unknown item
behind a boss or publishes a victory to the native game. Draygon still requires
Botwoon on the normal route, but Draygon's own post-victory exit is considered.

Bomb Torizo uses the Bomb location's approach plus the actual vanilla Bombs
wake condition or VARIA pickup wake patch. A sleeping Bomb Torizo, removed
Minimizer mini-boss, or Mother Brain in Disabled Tourian is hidden. In a boss
shuffle, diamonds stay on the actual boss room while accessibility follows the
shuffled entrances. No changes to the player's seed, placements or save are
required. Vanilla shows these only when its optional tracker is enabled;
speedrun tracker restrictions remain in force.

The original 100-item publication ABI remains intact. Unreal additionally
publishes ten identity-checked encounter states against the same snapshot.
Session, slot, inventory, seed, and objective mismatches reject the result;
pending or stale boss states are hidden rather than shown as falsely green.
The Interface settings legend explains the new shape.

## Reported regression

In seed 1008746831, the observed inventory had Morph, Bombs, Hi-Jump, Spazer,
399 maximum energy, 25 missiles, 5 supers and 15 power bombs. No boss flags were
set. Spore Spawn was reachable and beatable, but the old tracker displayed
only item checks, leaving its Gravity Suit reward red. The new encounter query
shows Spore Spawn green. After its native defeated flag is set, its marker is
gray and the Super Missile (pink Brinstar) item check becomes green. Taking only
the two available tanks does not unlock that route.

## Targeted validation

- `Scripts/test-boss-tracker-logic.py <captured-query.json>`: isolated queries
  cover that regression, all ten defeated flags, Draygon's victory-dependent
  exit, combat difficulty, shuffled boss routing, Disabled Tourian, and spoiler
  independence. The captured player request is kept only under ignored `.tmp`.
- `Scripts/test-boss-tracker-native.py <isolated-root>`: real C-core frames on
  gaming-pc; pause map and HUD minimap marker colors at 256 and 400 pixels,
  defeated/stale/invalid publications, and unchanged item totals. The map-render
  fixture enters the native pause transition directly; it is not a Start-button
  gameplay test during the boss entrance animation.

These are targeted checks, not a full playthrough of every seed and boss.

The actual Unreal async controller also passes `-SMTrackerSelfTest` on gaming-pc
for vanilla and two generated seeds with the ten-state boss publication ABI.
The original progression suite passed 316 queries across three complete seeds;
Minimizer cases covered 30, 72 and 100-check seeds.
