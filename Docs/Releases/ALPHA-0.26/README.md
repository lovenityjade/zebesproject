# ALPHA-0.26 — Face the Memory

Boss Rush, the Mother Brain finale and escape memories, forty new achievements,
room-editor improvements and corrected room extensions. English and Canadian
French remain selectable. Windows x64 ZIP and Linux x86-64 AppImage.

## Included

- Playable ten-encounter Boss Rush with Easy, Medium, Hard, Very Hard and Hardcore,
  progressive loadouts, native pause/failure menus, boss energy, splits and results.
- Optimized VR transitions, configurable rendering/transition quality and frame cap.
- Boss Rush encounter corrections for Crocomire, Phantoon and Draygon.
- Pixel-textured Hyper Beam and Mother Brain disintegration, Baby Metroid energy
  absorption, emergency lighting and Crateria fireballs. Face the Memory starts
  at 0:00 for the final duel and continues into escape without the Escape song.
- Seven 5.8-second escape memories with approved portraits, native font, automatic
  text and animated frames. Upper boxes cover the HUD; punctuation is corrected.
  Gameplay remains active. The shared escape hook covers Vanilla, NG+, Randomizer
  and the future Story path; this does not enable the unfinished Story campaign.
- Twenty Vanilla and twenty Randomizer achievements, including secrets.
- External editor improvements and the accepted room corrections. The imported
  room atlas is rebuilt from the player's ROM plus authored changes; its extracted
  PNG/BGRA source is excluded from release and public source.
- Normalized Boss Rush/finale audio in both packages. Windows retains the selected
  32-track Remastered pack. Audio provenance/credits distinguish AI-assisted music.

## Still planned

Story campaign, Multiworld, Boss Rush online rankings and the 150% speed category.
The newly requested [optional gameplay QoL](../../PLANNED-OPTIONAL-QOL.md) are
future work and ineligible for speedrun mode; they are not implemented here.

## Validation and update notes

Both packages were compiled and cooked with Unreal Engine 5.8.2 on dedicated
build hosts. Windows was compiled on Windows; its rendered checks ran under
Proton Experimental / D3D11 on gaming-pc. Linux used Vulkan on gaming-pc.

- Fresh extracted packages pass ROM, seed-sharing, profile and tracker self-tests
  using their packaged native libraries and Python runtimes.
- Linux also renders and blocks startup with a missing or altered ROM.
- Both packages render 21 French settings/randomizer pages and retain the chosen
  language. Graphics and seed-sharing pages were visually reviewed.
- Both cooked packages traverse all ten Boss Rush arenas, exercise Samus's death
  sequence and native Continue / Retry / End menus, then render the short escape
  fixture with its portrait dialogue, pixel Hyper Beam and fireballs. Both report
  zero emulated CPU opcodes. Pause and escape screenshots were visually reviewed.
  These are scripted fixtures, not human boss victories or a full escape run.
- All five Boss Rush audio assets are available from the bundled Content fallback
  on both platforms, including the non-looping failure and success recordings.
- Windows includes all 32 selected Remastered tracks, verified against their
  manifest. The packaged Windows DLL and Python play 240 frames of track 4 with
  nonzero PCM. This checks audio samples, not physical speaker output.
- Every packaged file was compared with a fresh extraction: 2,686 Linux files and
  1,746 Windows files. Download hashes were checked again after transfer.
- The room-atlas reconstruction was exercised through the native runtime and is
  pixel-exact against the authored reference, without altering emulated RAM.
  Malformed recipes are rejected. This is not a full room-by-room gameplay pass.
- Public-source GitHub CI passes native compilation and distribution gates.

Evidence: [Linux archive](linux-archive-audit.json),
[Linux package](linux-package-verification.json),
[Windows archive](windows-archive-audit.json),
[Windows package](windows-package-verification.json),
[Linux Boss Rush/escape](linux-feature-verification.json),
[Windows Boss Rush/escape](windows-feature-verification.json),
[room reconstruction](decor-runtime-verification.json).
Development evidence remains under Docs/BossRush, Docs/Finale, Docs/Achievements
and Docs/RoomEditor.

These are focused alpha checks, not complete playthroughs, exhaustive seeds or
certification of physical Windows GPUs. Back up saves/profiles before updating.
A compatible player-supplied ROM remains mandatory. No player saves or ROM are
included. Original runtime artwork is reconstructed from that ROM.

Downloads contain only the Windows ZIP, Linux AppImage and SHA256SUMS. Diagnostic
reports stay in this documentation. The public source excludes development
Scripts/Tests, ROMs, saves and extracted artwork references.
