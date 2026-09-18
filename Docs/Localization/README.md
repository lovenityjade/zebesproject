# French Canadian localization

Scope approved: every player-facing text in The Zebes Project (Vanilla,
Randomizer, native UI, modern UI, cinematics, messages, tracking, credits) and the
website including Help & FAQ. Preserve recorded voices and contributor names.
English stays selectable. No Google Translate or external translation service.

## Terminology

Prefer Nintendo Canada usage over France when verified. In particular Nintendo
Canada's Metroid Prime Remastered page uses **Morphosphère**, **combinaison de
puissance**, **rayon à ondes**, **viseur radioscopique**, and **trame sonore**:
https://www.nintendo.com/fr-ca/store/products/metroid-prime-remastered-switch/

Terms not verified in a Canadian source remain editorial choices; do not label
all chosen terms as official. Super Metroid-only equipment needs separate review.
Use manette, sauvegarde, options, plein écran, classement, succès in project UI.
Preserve seed numbers, settings strings, internal item/location IDs and profile
metadata exactly; localization is a presentation preference, never a ROM patch
on disk or a change to generation/logic.

## Implementation — September 18, 2026

- `Localization/fr-CA.tsv`: 1,387 manually authored shared translations.
- `Localization/site.fr-CA.tsv`: 431 manually authored website entries.
- `Scripts/build-localization.py` validates duplicates and printf/numbered
  placeholders and compiles the native/Unreal catalog. It does not translate.
- English remains the default. Choose **Language → French (Canada)**, displayed
  as **Langue → Français (Canada)** afterward, in the system menu or ROM setup.
  The choice is persisted in `[Localization] Language=en|fr-CA` in Presentation.ini.
- `SMLocalizedImGui.h` translates presentation arguments while retaining stable
  widget IDs, option values, settings strings and profile identifiers.
- Main settings, all 54 VARIA fields/descriptions, 105 techniques/descriptions,
  58 objectives, patches, validation messages, profiles, achievements, help and
  debug controls use the catalog. French search uses the displayed text.
- Native file/options, item/save messages, pause equipment, objective descriptions,
  map/tracker labels, dialogue glyphs, opening narration, cinematic captions,
  ending statistics and credit roles have French presentation paths.
- Native accented fonts are derived from the player's validated ROM at runtime.
  Temporary message/objective fonts are backed up and restored. The ROM on disk
  is unchanged; no extracted Nintendo font bitmap is added to the source catalog.
- Game Over has localized choices and a native-lettered French title panel over
  the existing illustration. Source artwork, particles and recorded music remain.
- Special Thanks adds **Guiz de Pessemier**, **twitch.tv/jeuserieux**,
  **Le Jeux c'est Sérieux**, **Eric Certossini**, **twitch.tv/certojeuxdroles**,
  **Certo Jeux Droles** before the decompilation credits. Contributor/project
  names and the mandatory Unreal trademark notice are retained.

## Website deployment

French home: https://thelovenityjade.me/zebes/fr/

French Help & FAQ: https://thelovenityjade.me/zebes/fr/help.html

English/French navigation, metadata, accessibility labels and dynamic leaderboard
messages are localized. The submission API, stored runs and runner names are
unchanged. `Website/localize.py` fails on missing prose; `Website/build.py`
checks output references. Production French HTML hashes matched the reviewed
build. The prior static site is backed up on nexus-vps at
`/var/backups/zebes-website/20260918T022048Z-fr-ca`.

## Verification and reproducibility

Builds and runtime checks use the disposable gaming-pc checkout
`/tmp/zebes-alpha024-release-20260916`, protected by `ISOLATED_TEST_DIRECTORY`.
The player's local game and binaries have not been replaced. The donor SRAM is
copied, never edited. Source ROM/SRAM hashes are checked after native tests.

- Native C library and Unreal Linux editor module compile successfully.
- `Scripts/test-localization-native.py`: English/French item/save prompts,
  unchanged RAM when switching language, restored temporary font VRAM and zero
  CPU opcode fallback.
- `Scripts/test-localization-files.py`: native copy/erase confirmations and
  cancellation; no copy or erase is committed. Slot-letter alignment inspected.
- `Scripts/test-localization-pause.py`: real map/equipment/objective transitions,
  restored font VRAM, unchanged inventory and area overview navigation.
- `Scripts/test-localization-intro.py`: original narrated sequence traversed with
  French text and native cue timing; recorded voices remain untouched.
- `Scripts/test-localization-captions.py`: Ceres/Zebes and escape captions.
- `Scripts/test-localization-credits.py`: requested thanks, native ending text,
  and preview exit restoring RAM and frame count exactly.
- Isolated Unreal Game Over review waits for native music/fade phase 4 before
  capturing the French title and both choices, including the flickering lamp.
  Use `-SMGameOverTest -SMTestSavedRoom -SMTestSave=<donor> -SMWarmup=9000`;
  the fixture copies the donor into a disposable save.
- `Scripts/test-dialogue-native.py`: accented pagination and reveal at native and
  widescreen widths, using the original font.
- Isolated Unreal `-SMMenuPreview=fr-review` visits 21 settings/randomizer pages.
  It saves/reloads the language and checks the native locale mirror
  (`SM_LOCALE_PERSISTENCE PASS`). Screenshots catch layout issues that builds
  cannot: sharing buttons now wrap, difficulty names and profile mode headings
  are translated.
- `Website/localization-check.mjs`: desktop/mobile home and help, language links,
  overflow, FAQ, empty/populated leaderboard and submission dialog, with no
  browser errors. Tests ran in disposable Chromium on gaming-pc.

Capture files and test reports remain in the disposable checkout's `SMTests`
and `Unreal/Saved/SMTests`; review copies are in local `.tmp/localization`.
Do not distribute fixture SRAM/ROM files or diagnostic screenshots as source.

## Practical limits

This is an alpha localization, not certification of every room, generated seed,
error path or resolution. Catalog coverage and successful automated checks do
not substitute for a complete bilingual playthrough. User-authored profile
names, seeds, URLs, technical identifiers and proper names are intentionally
preserved. No French voice recording is added.

The French website is live. The localized game has **not** been packaged into a
new Windows ZIP/Linux AppImage, published as a release, or installed over the
player's local build. Windows packaging/runtime validation remains necessary
before shipping these game changes.
