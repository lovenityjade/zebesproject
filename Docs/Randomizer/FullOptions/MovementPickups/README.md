# Respin, unrestricted Space Jump timing and item sounds

Public generation now supports `spinjumprestart`, `Infinite_Space_Jump` and
`itemsounds`, using the existing English Gameplay patches controls. Required
native behaviors are `respin-v1`, `infinite-spacejump-v1` and `item-sounds-v1`;
rule flags are 2048, 4096 and 8192. Options and behavior names must agree when
Unreal reads a manifest. Native capabilities now include bits 0–13.

## Source behavior

- **Respin:** the complete 253 pose tables are decoded from pinned VARIA
  `spinjumprestart.ips`, including its aiming-down and relocated-table changes.
  `Scripts/build-movement-pickup-data.py` emits native constant entries; no
  executable SNES patch bytes are installed. Original custom-controller
  translation still chooses entries. Changing from normal jumping/falling to
  spinning preserves vertical motion; the original takeoff cases still jump.
- **Infinite Space Jump:** source patch $90:A493 skips the air/water velocity
  windows. Equipment, descent, distinct jump press and liquid restrictions
  remain. The explicit seed patch takes priority over the separate buffered
  Space Jump assistance; seeds without this patch retain that preference.
- **Item sounds:** all 63 source PLM sound choices (21 items in three visibility
  forms) use their exact sound group/ID. The original item instruction dispatcher
  calls the native sound hook. Item pickup leaves queued background music intact;
  room-music restoration consumes the source $05D7 marker without restarting it.
  Unlisted music PLMs retain normal music behavior. The source's cancellation
  bypass and 32-frame minimum message delay are implemented; dismissal still
  waits for player input.

Vanilla sessions use original seed behavior. The original ROM and upstream
checkouts are not modified. `source-data.json` records source IPS hashes and the
independently inspectable pose/sound data.

## Targeted evidence

- `Scripts/test-native-movement-pickups.py`: 4,040 actual pose lookups, including
  remapped jump/run buttons; airborne/takeoff transitions with original and
  randomized sessions; 96 Space Jump equipment/descent/speed/input/assistance
  combinations; all 63 sound queues and music preservation; normal music fallback
  and 360/32-frame original/patched message delays. Zero emulated CPU opcodes.
- `Scripts/test-movement-pickup-seeds.py`: native API seeds 15093800 (all three)
  and 15093801 (Infinite Space Jump only), complete source solver/spoiler routes
  and 200 embedded tracker progression checks.
- `-SMMovementPickupSelfTest`: actual Unreal manifest parsing, independent A/B
  settings published and reloaded, C remaining vanilla, and rejection of an
  option/required-behavior mismatch.
- Native, Game and Editor builds pass. `Proofs/` records generated manifests,
  native/solver/tracker/profile results and the candidate binary hashes.

Execution was isolated on gaming-pc under
`/tmp/sm-native-generation-20260914/movement-pickups`. Its Game and Native/build
candidate are current. Earlier mode candidates and proofs remain separate.
No local launch or packaged-runtime/frozen-release replacement. The tests use
controlled native states and NullRHI profile validation; they do not claim
manual traversal, a new rendered capture or a listening test.

Remaining guarded options: Mirror, raceMode and animals. Full goal stays active.
