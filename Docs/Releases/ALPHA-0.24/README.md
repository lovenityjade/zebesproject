# ALPHA-0.24 — first public playtest

Release downloads: [Windows x64 and Linux AppImage](https://github.com/lovenityjade/zebesproject/releases/tag/ALPHA-0.24).
macOS is not included. Gameplay and the accepted title presentation are frozen;
release changes address portability, installation and ROM-derived resources.

## Validation

- **Windows:** native DLL and cooked Game compiled on Windows. The archive's ROM,
  seed-sharing, profile and tracker self-tests pass natively on Windows 10 x64.
  The top-level executable also passes the ROM bootstrap check.
- **Windows under Proton:** the final ZIP was extracted and tested through UMU
  1.4.1 / Proton Experimental on an RTX 3060, D3D11/DXVK. Packaged self-tests,
  missing/invalid ROM screens, title, save selection, loading a private save and
  brief gameplay/input/audio checks pass. Captures were visually inspected.
- **Linux:** the final AppImage runs through its actual read-only mount. ROM,
  seed-sharing, profile and tracker self-tests pass with bundled CPython 3.11.
  Missing/invalid ROM screens and title were also checked through the extracted
  AppRun. Actual mounted gameplay loads a private save, accepts input, renders
  correctly and produces non-silent engine audio. Data stays outside the image.
- Fixed a Linux packaging regression where bridge reload after settings
  preflight rejected its own already-loaded Python runtime. A standalone test
  covers repeated generation, determinism, and foreign/missing runtime rejection.
- **ROM reconstruction:** 49 world-patch transactions, all 256 HUD tiles and 85
  used objective-menu tiles match frozen fixtures. World, area, escape and animal
  room payloads are byte-exact after native ROM loading. All 26 tracker icons and
  the window helmet decode from the validated ROM; buffers are empty beforehand.
- **Source:** clean GitHub CI builds the native core without a ROM, verifies the
  known original-artwork gates and checks every vendored VARIA runtime hash.

Machine-readable reports in this directory identify the tested artifact hashes.
Build inventories deliberately retain `packagedGameTested: false`: that records
what the builder itself knows. The separate package-verification reports record
subsequent runtime results. Private gameplay screenshots/ROMs are not published.

## Distribution boundary

A valid player-supplied ROM is mandatory at every launch. Original game artwork,
fonts and room streams are decoded at runtime. Staging excludes ROMs, saves,
research media, upstream IPS, extracted tracker packs and optional recordings.
The supplied game-over recording is excluded; original ROM audio is the fallback.

Public source starts from a cleaned snapshot. Private development history and
old reference media stay private. The required VARIA Python/JSON subset is
vendored with revision/file hashes. The disassembly remains a credited research
reference, not a distributed binary-asset dependency.

`Scripts/check-distribution.py` checks known artwork boundaries and recipe hashes;
`Scripts/stage-runtime.py` uses an explicit runtime allowlist. These checks are
not general copyright classifiers. Project-authored code/docs are MIT; original
project artwork has separate terms and dependencies retain their notices.

## Known limits

- Alpha smoke tests do not certify every setting combination, physical route,
  controller, GPU, driver, save edge case or complete playthrough.
- Native Windows headless tests and rendered Proton tests are distinct evidence;
  Windows rendering on a broad hardware range remains community testing work.
- Linux ELF dependencies require glibc 2.35+. This is a measured symbol floor,
  not certification of every distribution at that version. Vulkan is required.
- Story Mode, Boss Rush and Multiworld are planned, not playable here.
- Online rankings and Discord login are not live. Local run features remain.
- Optional remastered recordings must be installed separately.
- Back up saves before updating. Report version, platform, seed and settings
  string; never attach a ROM or credentials.
