# Terminator green marker — 2026-09-15

Read-only diagnosis of the currently paused local game, PID 528022, slot A, seed 14092032. No game memory, save or seed settings were modified. Travel implementation remains paused.

The pictured green point is Energy Tank, Terminator: Crateria map tile (12, 7), original location address 492594. Its randomized reward was not inspected or revealed.

Live native inventory read from the process: acquired items 0x2104 (Morph, HiJump, SpeedBooster), no beams, max health 99, 5 missiles, no supers, power bombs or reserve. The published tracker state also identifies Terminator as the only green Crateria check.

The seed’s effective Casual rules enable SimpleShortCharge at difficulty 1; full ShortCharge is disabled. The native embedded oracle was queried on gaming-pc using the live inventory, actual opened-door flags, collection flags and effective seed settings. Results are in result.json:

- Current settings: green, reachable and safe, difficulty 1, technique SimpleShortCharge, item SpeedBooster.
- Same request with only SimpleShortCharge disabled: red/blocked.
- Same request with only SpeedBooster removed: red/blocked.

VARIA graph/vanilla/graph_locations.py defines Terminator access from Landing Site through canPassTerminatorBombWall(). graph/vanilla/graph_helpers.py allows SpeedBooster plus SimpleShortCharge/ShortCharge from that side, or bomb-wall destruction equipment. Return from the opposite side allows SpeedBooster without the short-charge technique. The preset technique description in utils/parameters.py defines SimpleShortCharge as accelerating activation by delaying the run-button input.

Conclusion: the green state matches the seed rules; it assumes a technique the UI currently does not explain. This verifies oracle behavior, not a manual traversal demonstration or runtime collision parity for this specific route. No preference change has been applied.
