# Achievement expansion

40 additions: **20 Vanilla**, **20 Randomizer**, alongside the existing five
common milestones. The [catalog](catalog.json) includes all names, English and
French descriptions, stable IDs, categories, badge indices and secret flags.
It contains spoilers for the seven secret achievements.

Achievements remain local to a save bank. Different slots contribute to the
bank's collection, but each individual condition reads one current save; item
counts, bosses and inventory are never summed across slots or seeds.

The menu has category selection, independent completion counts, illustrated
badges and masked secret entries. A locked secret hides its name, description
and badge. Unlocked milestones use the existing native toast/sound queue.

Twenty badge illustrations are shared by related milestones across categories
(for example Kraid uses the same emblem in Vanilla and Randomizer). Their
generation prompts explicitly contain `SNES 16-bit era super metroid style`.
Generated artwork is temporary, as disclosed by the game's existing startup
notice; it is not presented as extracted Nintendo art.

## Conditions and compatibility

- Inventory and boss milestones can be recognized from an existing save.
- Acquisition-order secrets require an observed change in the same slot and
  session; loading an existing inventory is not treated as proof of order.
- Randomizer totals exclude locations disabled by Minimizer. Fixed thresholds
  (10, 25, 50) retain their stated meaning; not every seed supports every badge.
- Tablet achievements require an active hunt and use its configured **required
  quota**, not the number of tablets placed. Half of an odd quota rounds up.
- Finishing means entering the real escape/ending after gameplay. A debug
  ending preview or direct debug credit roll is not a completion.
- Speed milestones use the native in-game clock. They are casual local
  milestones, not verified speedrun leaderboard submissions.
- Legacy `Local/Unlocked` bits 0–4 remain intact. `Local/Unlocked64` stores
  the expanded 45-bit collection; unknown bits are discarded on load.
- ROM and SRAM layouts are unchanged. Icons are staged as external runtime
  content; no ROM artwork is added to the distribution by this feature.

Validation completed on gaming-pc in an isolated checkout:

- [Rules](rules-verification.json): all 40 additions, mode separation, seven
  secrets, quota/time boundaries, acquisition order and session/slot isolation.
- [Native integration](native-verification.json): real equipment/boss flags,
  the highest achievement bit in the toast queue, debug ending exclusion and
  unchanged simulation memory and reference ROM/save files.
- [Unreal integration](runtime-verification.json): legacy migration, 64-bit
  persistence, awards from the native state, all twenty badge textures and both
  category screens. Rendered menu and toast captures were inspected.

The Linux editor module and native library were built remotely and installed
locally for the next launch, with backups in `.tmp/achievements/before`.
No local game was launched and no release was published. Windows has not been
rebuilt for this change. These checks do not replace full playthrough testing.

Artwork: `Unreal/Content/Achievements/`. The original generated PNGs are retained;
the runtime uses 64 × 64 menu textures and 32 × 32 toast badges. The complete
[prompt set](imagegen-prompts.json) records the built-in ImageGen workflow.
The [readable catalog](CATALOG.md) lists the forty additions and contains spoilers.
