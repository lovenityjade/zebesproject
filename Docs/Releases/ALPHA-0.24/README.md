# ALPHA-0.24 — first public playtest

Windows x64 and Linux x86-64 AppImage are the release targets. macOS is not
included. Gameplay and the accepted title presentation are frozen for this alpha;
release work focuses on portability, installation and ROM-derived resources.

## Validation

- Windows: native DLL, Game and Editor compile on Windows. Three seed profiles
  produce complete spoiler/progression manifests and tracker results. UTF-8 save
  paths and atomic file replacement pass native checks.
- Windows cooked runtime: title and save selection rendered correctly through
  UMU/Proton Experimental on an RTX 3060. ROM, settings sharing, profile and tracker
  packaged self-tests pass on the earlier private candidate. Final package
  verification is tracked separately below.
- ROM reconstruction: 49 world-patch transactions, all 256 HUD tiles and 85 used
  objective-menu tiles compare exactly with the frozen fixtures. World, area,
  escape and animal-room payloads match byte for byte after native loading.
- All 26 tracker icons and the window helmet are decoded from the validated ROM.
  Icons use original pickup frames/statue portraits rather than the reference
  pack's retouched sprites. Original native output was visually inspected.
- The clean AppImage and Windows archive still require their final packaged
  tests before publication. This document is updated with the release evidence.

See the machine-readable `*-verification.json` and `*-reconstruction.json`
reports in this directory. A successful build or headless check is not proof of
complete gameplay, audio quality, or compatibility with every GPU.

## Distribution boundary

A valid player-supplied ROM is mandatory at every launch. Original game artwork,
fonts and room streams are decoded at runtime, not stored as exported image
arrays. Runtime staging excludes ROMs, saves, research media, upstream IPS,
extracted tracker packs, and optional recordings. The supplied game-over recording
is excluded as well; the original ROM audio remains the fallback.

The public source starts from a cleaned snapshot. Private development history
and its old reference media stay private. VARIA's required Python/JSON subset is
vendored with upstream revision and file hashes; the disassembly is a credited
research reference, not a binary-asset dependency distributed with this release.

`Scripts/check-distribution.py` verifies known runtime artwork boundaries and
recipe provenance. `Scripts/export-public-source.py` prepares a fresh source tree;
packaging uses `Scripts/stage-runtime.py`. None is a general copyright classifier.

## Known limits

- This is a prerelease for bug testing. Randomizer option combinations and
  physical routes are not exhaustively playtested.
- Windows rendering was tested under Proton; native Windows Editor presentation
  was checked separately. No claim of broad Windows hardware certification.
- Story Mode, Boss Rush and Multiworld are planned, not playable in this version.
- Online rankings and Discord login are not live. Local run features remain.
- Optional music must be installed separately; no music packs are downloadable
  from this release.
- Back up save/profile data before updating. Report version, platform, seed and
  settings string; never attach a ROM or credentials.

Project-authored code/docs are MIT. Original project artwork has separate terms.
Third-party notices and credits remain with the distribution.
