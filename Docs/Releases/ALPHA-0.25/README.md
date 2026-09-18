# ALPHA-0.25 — French Canadian and presentation update

September 18, 2026. Windows x64 ZIP and Linux x86-64 AppImage.

## Included

- French Canadian game and website text, with English still selectable. Choose
  **Language → French (Canada)** in system settings. Recorded voices and names
  remain unchanged. See [coverage and development checks](../../Localization/README.md).
- Requested Special Thanks for Guiz de Pessemier and Eric Certossini.
- Seed-dependent map layout, saved-station travel and refills enabled by default
  for fresh configurations. Existing preferences are retained.
- Suit transformations, Maridia edge cleanup, Mother Brain widescreen clipping,
  planetary escape presentation and the native dialogue foundation.
- Windows retains the DLL fixes and the 32 selected, credited Remastered tracks.
  Linux uses original ROM audio unless an optional music pack is installed.

## Package verification

Both platforms were compiled and cooked with Unreal Engine 5.8.2 on dedicated
build hosts. Windows graphics were tested under Proton Experimental on gaming-pc;
this is not certification of a physical Windows GPU.

- Fresh extracted downloads pass ROM validation, seed-sharing, profile and
  tracker self-tests using their packaged native library and Python runtime.
- Linux also renders and blocks startup with a missing or modified ROM.
- Both packages render 21 French settings/randomizer pages and pass language
  persistence checks. Graphics and randomizer screenshots were visually reviewed.
- Windows Remastered manifest verifies all 32 track hashes. Its packaged DLL and
  embedded Python play 240 frames of the title track with nonzero PCM and zero
  emulated CPU opcodes. This is a PCM check, not a speaker recording.
- Package files were compared with fresh extractions. Linux's desktop entry has
  the expected version field added by appimagetool. No ROM, save or diagnostic
  capture is included. Original runtime artwork requires the player's ROM.
- Public-source GitHub CI passes native compilation and provenance checks.

Evidence: [Linux](linux-package-verification.json),
[Windows archive](windows-archive-audit.json),
[Windows runtime](windows-package-verification.json).

These are focused alpha checks, not complete playthroughs or exhaustive seed,
resolution and hardware coverage. Story, Boss Rush and Multiworld remain future
work. Back up the complete save/profile folder before updating.

The release contains only the Windows ZIP, Linux AppImage and `SHA256SUMS`.
The public repository retains its clean history and excludes development
`Scripts`/`Tests`, ROMs, saves and extracted reference assets. Website screenshots
are presentation material and are not used as runtime game assets.
