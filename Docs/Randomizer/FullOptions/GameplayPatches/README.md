# Native gameplay patches

Implemented 2026-09-15 from the pinned VARIA assembly sources:

| Configurator option | Native behavior | Flag |
| --- | --- | --- |
| `relaxed_round_robin_cf` | Crystal Flash accepts fewer than 10 of each ammo; consumes up to 30 total, cycling available types every eighth frame, restoring 50 health each. Original health/reserve/input conditions remain. | 64 |
| `rando_speed` | Source landing poses preserve horizontal momentum, including subpixel speed. Stationary landings, shooting poses and the original no-shot-direction exception retain their original behavior. | 128 |
| `nerfedCharge` | Charging without the upgrade gives one-third charged damage, 66 base pseudo-screw damage, and a three-Power-Bomb SBA cost. Actual Charge restores normal damage/cost; SBA damage remains original. | 256 |

The existing English Gameplay patches controls now generate these options through
the public API. Required behavior names are included in manifests and checked
against effective options by Unreal. Independent slot plans restore the flags;
vanilla sessions disable all seed rules. Capabilities now include bits 0–8.
No upstream files or frozen Save Refill release artifacts were changed.

## Targeted evidence

- `Scripts/test-native-gameplay-patches.py`: original native entry, movement,
  firing and contact routines after boot on gaming-pc. Six CF eligibility cases,
  three full consumption traces, 144 landing combinations, six charged beam
  combinations, charge-without-upgrade, pseudo-screw contact and SBA cost/damage.
  Vanilla deactivation and zero emulated CPU opcodes verified.
- `Scripts/test-gameplay-seeds.py`: native API seeds 15093600 (all three enabled)
  and 15093601 (nerfed Charge only), complete solver/spoiler routes and 200
  progression queries through the embedded tracker. Source NerfedCharge and
  RoundRobinCF logic flags agree with effective settings and tracker settings.
- `-SMGameplayProfileSelfTest`: actual Unreal parser and profile publication,
  reload of separate A/B seeds with different flags, C remaining vanilla, and
  rejection of inconsistent effective options/required behaviors. The focused
  entry point avoids repeating the old profile suite.
- Native, Game and Editor builds passed. `Proofs/artifacts.json` identifies
  binaries and records frozen-release/upstream verification. Other proof files
  retain native results, generated manifests, progression and Unreal logs.

All execution was isolated under `/tmp/sm-native-generation-20260914` on
gaming-pc. The isolated Game binary and Native/build library were updated;
the proven Escape library remains separately available under `escape/`.
No local launch or packaged runtime replacement occurred. The native probes use
controlled RAM conditions, not a manual playthrough. Profile checks use NullRHI;
no new visual claim is made for these behavior-only changes.

Full integration remains incomplete: Mirror, race and other outstanding
noncosmetic patches are still part of the active goal.
