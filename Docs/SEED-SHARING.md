# Random seed numbers and shared settings

In **Randomizer → Seed & presets**, leave **Seed number** empty to choose a number when **Generate Game** is selected in the native menu. Explicit numbers must be in `1..2147483647`.

A blank draft persists as seed `0`. On Generate Game, `FSMProfiles::ResolveSeed` reads the slot's saved draft, chooses a positive number and saves it before starting the worker. Retries use that saved number. Merely opening/editing the menu, copying settings or saving a draft does not draw a number. Generated A/B/C slots retain their own numbers and fingerprints. Existing numbered drafts remain unchanged.

## Sharing

- **Copy settings** copies the current draft configuration to the desktop clipboard.
- Paste a string into **Settings string**, or use **Paste**, then select **Import settings**.
- **Games & Saves** also provides **Copy settings** alongside each randomized A/B/C slot, including generated slots.
- A string carries skill/progression, all option objects and random pools, techniques, combat/heat tolerances, patch selections, No Advanced Techs, and Relic Hunt enablement/quotas.
- The seed number, save identity and local graphics/audio/input preferences are excluded. Import keeps the seed field unchanged. Share the number separately to reproduce the same world on the same generator version; leave it empty for another world with the shared configuration.
- Import validates a temporary request before replacing the draft. Invalid input preserves the prior settings. Generated saves are immutable; importing changes the editable draft, not their contents.

## Format

`ZP1.` followed by unpadded URL-safe Base64. The decoded envelope contains a four-byte little-endian uncompressed length, a four-byte CRC32 of the JSON bytes, then zlib-compressed UTF-8 JSON. The payload has schema 1 and exactly these fields: `schema`, `skill`, `progression`, `noAdvancedTechs`, `relicHunt`, `patches`, `options`, `techniques`, `skillSettings`.

Limits: 100,000 characters encoded and 65,536 bytes decoded. Versions, checksums, field shapes and supported option values are checked before applying. This is the port's sharing format; existing VARIA JSON import remains separate. Defaults/skill presets are tied to the bundled generator catalog, so compatibility with future changes requires an explicit version/migration decision.

## Verification — 2026-09-15

Built standalone and Editor targets. Tests ran on gaming-pc in the isolated test root:

- Blank/explicit/invalid/overflowing number inputs.
- Random number variation, including rapid successive requests.
- Full settings round-trip with custom techniques, tolerances, patch selection, No Advanced Techs and relic quotas; imported seed number remains unchanged.
- Corrupt/truncated/unknown-version/oversized strings rejected with the previous request preserved.
- Two blank requests generated positive, distinct seeds (`1032809128`, `800788256`). Sharing/importing the settings and reusing the first number reproduced fingerprint `95a02a476fcad493a8b793a17eeaf20c002f3a78e6ad90dde646a823d3532b27` exactly.
- Profile self-test: blank draft survives save/reload; first resolution persists; retry keeps its number; existing independent-slot generation/copy/recovery regressions pass.
- Seed & presets UI screenshot inspected: empty-number hint, settings string field and Copy/Paste/Import controls visible.

Self-test flags: `-SMSeedSharingSelfTest=<isolated-root>` and `-SMProfileSelfTest=<isolated-root>`. Evidence: `.tmp/seed-sharing/`. No local game launch was performed.
