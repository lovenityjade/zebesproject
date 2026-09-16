# ALPHA-0.24 release checkpoint — September 16, 2026

The owner authorized Windows and Linux publication and a public repository.
macOS is excluded. The accepted gameplay and presentation remain frozen.

## Delivered artifacts

- Windows x64 ZIP, cooked on Windows and checked natively plus UMU/Proton.
- Linux x86-64 AppImage, cooked and checked on gaming-pc through its real mount.
- Artifact hashes, archive audits and scoped runtime verification reports.
- README with logo, features, roadmap, contributor credits and installation.
- MIT project code/docs, separate artwork terms and dependency notices.

See [validation and limitations](README.md) for the final evidence. The Linux
bundled-Python reload regression is fixed. Both packaged games pass ROM,
seed-sharing, profile and tracker checks, plus rendered startup/save/gameplay and
non-silent audio smoke checks. No full-run or exhaustive hardware claim is made.

## Distribution work completed

Original HUD, objective-pause, tracker/window icons, Ceres tile pattern and mixed
room data now decode from the player's validated ROM. World/area/escape/animal
streams and used UI tiles were checked against frozen private references.
The unused Mirror payload was removed. Unknown-rights recordings, research
screenshots, ROMs, saves and upstream graphics/IPS are excluded from staging.

The public repository starts from a clean snapshot with only the native-core
submodule and a hash-pinned VARIA Python/JSON runtime subset. It does not inherit
private development commits, tags or their historical reference media.
The original repository is preserved as `zebesproject-development-private`.

Workspace checkout used for public commits: `Build/PublicSource-ALPHA024`.
The outer development checkout continues to point to the private archive.
Keep these histories separate during subsequent maintenance.

## Release locations

- Source: https://github.com/lovenityjade/zebesproject
- Downloads: https://github.com/lovenityjade/zebesproject/releases/tag/ALPHA-0.24
- Required player ROM: 3,145,728 bytes, CRC32 `D63ED5F8`.
- Public source CI builds the native core and audits provenance; it does not
  claim to build Unreal platform packages on GitHub-hosted runners.

Private screenshots and audio captures remain outside public source. Accepted
local installations and player saves were not overwritten by these tests.
