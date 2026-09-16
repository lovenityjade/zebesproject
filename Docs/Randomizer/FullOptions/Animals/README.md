# Animals Surprise — native integration

All ten pinned VARIA variants are captured from the actual source random draw.
The native port does not redraw, omit variants, or apply executable ROM bytes.
The `animals` option is accepted by the full English configurator's existing
settings path. Source-incompatible Mirror/escape-randomized requests are ignored
by VARIA and recorded as effective `off` with an adjustment, preserving the
requested setting separately.

`Scripts/build-animals-data.py` captures 602 data bytes across all ten variants,
with exact source hashes and separately recorded code regions in
`Randomizer/native_animals.json`. Room headers, door descriptors, door lists,
PLM/enemy definitions and compressed tiles are data. Native callbacks implement
pre-Bomb-Torizo routing, boss resets/return doors, Draygon/Ridley tiles and BTS,
Phantoon/metal-pirate grey-door resets, the 20-second BCD timer, early ending with
native stat finalization, hostile Etecoon properties, and the escape-event change
for Alcatraz bomb blocks. Null source callbacks remain null; Draygon retains its
original pause/unpause hooks. No 65816 interpreter is introduced.

`sm_animals` configurations are pending per slot until a valid item plan commits.
All affected ROM bytes are restored on slot changes or Vanilla selection.
Generation carries a typed mode/name/catalog contract in `nativeContext`, covered
by the seed fingerprint and `native-animals-v1` requirement. Unreal validates
that contract against effective settings, persists it through existing immutable
profiles, and configures/clears native mode during generation and slot loading.
Historical plans default to mode zero; original SRAM layout is unchanged.

## Evidence and limits

- `Proofs/native-results.json`: all ten variants match the source data byte for
  byte. Actual room/door dispatch, escape-event flags, timer/ending state,
  Etecoon initialization, pending/rejected plans and full ROM restoration pass
  on gaming-pc. Zero emulated CPU opcodes.
- Seeds 15093900 and 15093901 both draw `draygonimals.ips`; both complete the
  authoritative solver route and 100 embedded tracker checks each. Seed
  15093902 confirms the actual source ignores Animals Surprise when Escape
  Randomization is enabled, and passes its full solver progression.
- `Proofs/profile.log`: `SM_ANIMALS_SELF_TEST PASS`. Actual Unreal ReadPlan and
  independent A/B profile save/reload, C remaining Vanilla, and rejection of an
  effective-option mismatch. Both generated seeds happen to have the same
  animal variant; this profile test does not claim different variant draws.
- Native, Game and Editor builds pass. The isolated gaming-pc runtime under
  `/tmp/sm-native-generation-20260914` is updated; prior mode binaries/proofs
  are retained. No local launch or packaged/frozen-release replacement.

Controlled callbacks establish data and behavior, not manual traversal of all
animal rooms or a new Vulkan image. Race and full Mirror remain unfinished.

Native candidate SHA256: `2b98185d59368d4956ef45f221d7fb9bb720a73f6a203ce3ddf08d631949b572`.
