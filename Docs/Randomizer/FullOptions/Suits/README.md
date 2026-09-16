# Suit behavior options

The public `gravityBehaviour` option now accepts Vanilla, Balanced and Progressive
with actual native damage behavior. Existing English controls already contain
these choices. New manifests require `native-suits-v1`; native rule flags are
512 for Balanced and 1024 for Progressive (Vanilla uses neither).

| Mode | Enemy / Metroid damage divisor: neither, Varia, Gravity, both | Periodic damage divisor | Suits blocking heat |
| --- | --- | --- | --- |
| Vanilla | 1 / 2 / 4 / 4 | 1 / 2 / 4 / 4 | Either |
| Balanced | 1 / 2 / 4 / 4 | 1 / 4 / 1 / 4 | Varia |
| Progressive | 1 / 2 / 2 / 4 | 1 / 2 / 2 / 4 | Varia |

Source: pinned `patches/common/patches.py` entry
`Removes_Gravity_Suit_heat_protection`, and
`patches/common/src/progressive_suits.asm`. Balanced changes the periodic damage
BIT operand from $20 to $01, so Varia quarters periodic damage. Progressive
changes normal enemy damage and the separate Metroid fractional drain routine.
The original fractional truncation and freeze behavior remain in the native
periodic-damage routine. Original heat palette selection remains intact.

**Correction to the investigation:** old tested manifests use Vanilla, because
the resolver overlays VARIA's vanilla preset onto its initial defaults. Balanced
was guarded before this work. There is no evidence here of old generated seeds
having a Balanced damage mismatch. Their fingerprint and SRAM remain unchanged.
The Unreal compatibility probe explicitly checks an existing pre-suits manifest
and retains its Vanilla rule flags.

## Targeted evidence

- `Scripts/test-native-suits.py`: 24 mode/equipment/session combinations,
  96 fractional periodic damage cases, heat immunity, enemy damage and Metroid
  fractional drain through the real native routines. Vanilla sessions ignore
  configured seed flags. Zero emulated CPU opcodes.
- `Scripts/test-suit-seeds.py`: native API seeds 15093700–15093702, one per mode,
  complete source solver/spoiler progression and 300 embedded tracker checks.
  Source NoGravityEnvProtection/ProgressiveSuits flags are checked explicitly,
  not merely compared between manifest and tracker.
- `-SMSuitsProfileSelfTest`: Unreal reads all three actual manifests, publishes
  independent A/B/C plans, reloads their different modes, rejects invalid suit
  settings and reads a historical Vanilla manifest unchanged.
- Native, Game and Editor builds pass. `Proofs/` contains manifests, detailed
  results, profile/build logs and candidate binary hashes.

Execution took place only on gaming-pc under
`/tmp/sm-native-generation-20260914`. The isolated Game and Native/build module
are current; the earlier Escape and gameplay-patch candidates remain separately
preserved. No local launch, packaged runtime replacement, upstream edit or frozen
Save Refill release edit. Native tests use controlled RAM after real boot;
profile checks use NullRHI and do not claim a new rendered playthrough.

Remaining guarded choices: Mirror, raceMode, itemsounds, spinjumprestart,
Infinite_Space_Jump and animals. The full integration goal remains active.
