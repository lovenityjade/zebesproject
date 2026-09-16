# Active release work — September 16, 2026

The owner authorized publishing both Linux and Windows releases and making
`lovenityjade/zebesproject` public. macOS is excluded. Linux delivery is an
AppImage. Windows may be tested on gaming-pc through UMU/Proton Experimental.
No public release or visibility change has happened yet.

The no-Nintendo-assets requirement, mandatory ROM gate, MIT project-code license
and reserved original-artwork rights remain in force. Publication authorization
does not waive those requirements. Preserve the accepted local install and saves.

## Order of work

1. Preserve the accepted freeze and current portability work locally.
2. Finish the isolated Windows cook and run the real Windows executable through
   UMU/Proton Experimental on gaming-pc, with its own prefix and saves. Record
   rendering, ROM-gate and randomizer evidence; distinguish it from native Windows.
3. Replace remaining extracted artwork with genuine runtime ROM decoding;
   remove unused Mirror payload (the owner excluded Mirror), audit mixed world
   data, and verify visual/gameplay parity. Do not bypass the distribution gate.
4. Exclude recordings with unconfirmed redistribution rights from public output;
   keep optional local soundtrack loading and the original ROM audio fallback.
   Do not claim ownership or invent permission. Audit tracker-reference provenance.
5. Stage a clean runtime allowlist and build/test Linux AppImage and Windows
   archive. Validate with missing/invalid/valid ROM and persistent save banks.
6. Sanitize public source and history. Retain the private original history as a
   backup; no extracted reference sprites, private ROM, user data or credentials
   may become public through an old branch/tag.
7. Publish the tested prerelease with checksums, full notices/credits, platform
   requirements and known issues, then make the clean repository public.

## Current evidence and issues

- Windows DLL, Game and Editor build; native generation and headless Unreal tests
  pass. Startup warnings and title are visually confirmed on RX 580 / D3D11.
- Windows save-selection capture is black except version text even after delayed
  capture. Do not mark that screen validated based on the state-machine PASS.
- AppImage launcher/storage stub test passes on gaming-pc. Bundled CPython seed
  fingerprint matches the Windows/native baseline. Unreal ROM self-test passes
  with non-ASCII UserDir. No actual AppImage exists yet.
- Distribution gate still rejects tracker/HUD/pause pixels, helmet BMPs,
  unused Mirror payload and mixed world data. The old Linux package is dirty.
- Windows private test staging is not distribution clearance or a release archive.

## Runtime/asset checkpoint

- The cooked Windows build runs through UMU 1.4.1 and Proton Experimental
  (experimental-11.0-20260903c-x86_64), D3D11/DXVK, RTX 3060. Title and save
  selection were visually inspected and are correct. Four packaged headless
  checks pass: ROM, seed sharing, profiles and tracker. A missing pair of empty
  VARIA IPS directories was found and corrected in the private stage.
- Mirror byte payload is removed; enabling Mirror is rejected. All 65,805 world
  patch bytes reconstruct exactly from the ROM plus 398 VARIA room edits and
  988 functional record bytes. 49 actual native ROM transactions pass.
- HUD and objective-pause sheets now load original tiles from the ROM. Only
  reviewed VARIA lettering, counters and objective symbols remain authored
  additions. All 256 HUD tiles and all 85 used pause tiles match the frozen
  presentation byte for byte. Both runtime sheets are empty before ROM load.
- Tracker icons and startup helmet remain the outstanding known graphic gates.
  Unknown GameOver recording, source/reference history and clean staging still
  require completion. Neither the private test tar nor old Linux stage is a release.

- Added a shared runtime staging allowlist (Python/JSON services and presets,
  explicit licenses; no upstream web assets or IPS). This narrowed allowlist
  still needs a packaged test; the prior passing private stage was broader.
- Game Over PCM is no longer a cooked runtime dependency. Optional audio paths
  use the same writable data root as the ROM, including AppImage installations.
  The accepted local install/recording remains untouched. These latest C++ path
  changes still need final platform builds and runtime checks.
- The Ceres parallax checker now samples its ROM-loaded Mode 7 tile, instead of
  embedding the 64 palette-index pixels. Native rebuild/visual check pending.

## Final package checkpoint

- Actual native reconstruction passes for world, area, escape and animal room
  payloads. Ordinary generators reproduce all recipes and preserve catalog IDs.
- Tracker icons (26) and helmet now decode from ROM; native buffers are empty
  before loading. Original atlas reviewed, including all boss portraits.
- Startup resources use the project-owned logo, with a hash-bound provenance
  manifest. Both Linux and Windows Game builds pass with runtime window icons.
- PopTracker's sector table was replaced by native semantic state selection;
  compatibility colors remain. No external renderer is linked.
- Distribution's known artwork gate passes. Both final cooks have been started;
  Windows cooked successfully. Actual AppImage/final archive tests remain.
- Public export preserves private history, removes reference media/disassembly
  assets and vendors only the licensed VARIA runtime Python/JSON subset.
