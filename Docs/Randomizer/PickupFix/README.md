# Hidden / Chozo Morph Ball grant correction

2026-09-15. Reported while testing seed **14092032**, native slot A.

## Confirmed cause

The seed places Morph at **Energy Tank, Brinstar Ceiling**, the hidden ceiling
check in Blue Brinstar Energy Tank Room (`8F:9F64`), location PC `07879E`,
collection bit 29. This was not a Chozo statue pickup.

The original ROM's unused Morph variants both have incorrect equipment operands:

| Variant | PLM | Equipment operand | Message | Original result |
| --- | --- | --- | --- | --- |
| Visible | `84:EF23` | `84:E416 = 0004` | 9, Morph Ball | Morph |
| Chozo | `84:EF77` | `84:E8CE = 0002` | 9, Morph Ball | Spring Ball |
| Hidden | `84:EFCB` | `84:EE02 = 0002` | 9, Morph Ball | Spring Ball |

VARIA already corrects both operands in
`Randomizer/upstream/patches/common/src/vanilla_bugfixes.asm:70`. Our native
item-placement integration had omitted this native equivalent. The correct
sprite and message therefore did not prove a correct grant. The embedded
tracker was correctly reporting the wrong inventory actually received.

`Native/sm_seed.c:sm_seed_equipment_mask` now returns Morph's bit `0004` for
those two PLM identities in randomized sessions. The generated bank-84 overlay
calls it from the original equipment-grant instruction. Other items and the
Vanilla path keep their original operands. Source ROM, original graphics,
message, location bit and upstream source remain unchanged. Existing seeds
use the correction without regeneration or a different seed fingerprint.

## Reproduction and validation

- [Original bug reproduction](reproduction-before.log): original library,
  actual native Hi-Jump pickup, then the actual Blue Brinstar ceiling room.
  A normal upward shot reveals the block and a normal jump collects it.
  Message **9** is displayed, acquired inventory becomes **0102**.
- [Corrected ceiling pickup](ceiling-verification.json): identical seed and
  input sequence, message **9**, acquired/equipped inventory **0104**, exactly
  the two expected collection flags, zero emulated CPU instructions.
  Positioning and temporary invincibility are test fixtures; block reveal,
  projectile collision, jumping and item collision run native game code.
- [All 63 grant paths](variant-verification.json): all 21 items in Visible,
  Chozo and Hidden variants. Native PLM dispatch checks the actual inventory
  delta, item message, collection bit and tracker inventory snapshot. These
  fixtures enter the authored collection script; they do not claim to traverse
  all 63 containers or rooms. Rendering remains enabled for this matrix.
- [Save recovery verification](recovery-verification.json): the copied saved
  slot loads through native SRAM validation, owns Hi-Jump and Morph, retains
  exactly the two collected checks, and gains both expected early missile
  routes under the seed's live tracker rules.
- [Four complete seed replays](progression-verification.json): seeds 14092026,
  14092027, 14092028 and the actual incident seed 14092032, **400 native item
  grants** in their verified solver progression order. Before every grant, the
  live tracker evaluates the actual accumulated native inventory and the
  seed's effective settings. Each next check must be reachable (green or
  advanced/uncertain-return yellow); previous checks must be cleared. Every
  grant must match the manifest's item, native quantity/bit, message and actual
  location flag. All 100 flags are checked at the end of each seed.
  Boss flags follow the solver log; traversal and combat are fixtures. The
  native turbo render-skip is used after boot for these logic replays, so this
  evidence is not a 400-location graphical or controller-driven playthrough.
  The 63-variant matrix and ceiling reproduction above retain rendering.

Tests run only on **gaming-pc**, in the marked isolated directory
`/tmp/sm-native-generation-20260914`, without an interactive game window.

![The actual corrected ceiling pickup](ceiling-morph.png)

![Native equipment after save recovery](recovered-equipment.png)

## Affected save

Profile `01A0A4746B647C6FAA0A30CAAC21B373`, slot A profile
`01A0A474FC0578C085BB1A2D36115161`, seed fingerprint
`f9cc2f3e2a205ad2f62253df56e97419df2a3d7be229b936bfd09bf60240133d`.

The preserved bank has exactly two collected locations: Morphing Ball
(Hi-Jump) and Energy Tank, Brinstar Ceiling (Morph). Acquired and equipped
inventory are `0102`, proving the erroneously granted Spring Ball was saved.
The seed's actual Spring Ball location, Energy Tank, Kraid, is uncollected.

An intact copy of the entire profile is preserved permanently at
`Unreal/Saved/SM/Recovery/20260915-hidden-morph/profile-before`, as well as in
`.tmp/pickup-incident-20260915/original-profile`. The verified recovery changes
only acquired/equipped `0102` to `0104` and slot A's four checksum words.
No item location flags, other slots, room, resources, play time, seed or stats
are changed. After all four seed replays passed, the recovery was applied
atomically to the live bank with the game closed and an exact comparison to
the original backup. [Applied recovery evidence](applied-recovery.json)
records the original and repaired hashes and all six changed byte offsets.
The repaired run can resume in slot A without regenerating the seed.

The previous map-selection work is recorded in
[AREA-SELECTION-WIP.md](../../Tracker/AREA-SELECTION-WIP.md) and remains paused.
