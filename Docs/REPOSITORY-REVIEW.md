# Initial repository review — 2026-09-15

**Superseded distribution boundary:** see [ROM setup and asset blockers](ROM-SETUP.md).
The private import contains pre-extracted artwork; exports and pushes are now
blocked pending migration. The earlier local-ROM packaging opt-in is removed.

This is a focused first-import review, not a claim that every gameplay path or
all historical changes have received an exhaustive audit.

## Findings addressed

- **Private data in distribution output:** stage-runtime copied the developer's
  ROM unconditionally and recursively copied research/runtime diagnostics.
  The ROM now requires explicit `SM_INCLUDE_LOCAL_ROM=1`; documentation uses
  an allowlist. Existing output containing a ROM or legacy documentation is
  rejected before modification. This preserves old artifacts and prevents a
  seemingly clean export from silently retaining them.
- **Fresh checkout packaging:** package.sh and prepare-assets.sh set TMPDIR to a
  directory that might not exist. Both now create it first. Primary build/play/
  packaging scripts use SM_ENGINE or a home-relative engine installation path,
  instead of the original author's absolute home directory.
- **Repository boundary:** builds, release snapshots, ROMs, saves, Python caches,
  raw capture buffers and most screenshots are ignored, preserved locally.
  Source catalogs, generator inputs, native generated includes and reference
  licenses remain versioned. Historical fixture seeds are retained as fixtures.
- **Dependency reproducibility:** native-core, disassembly reference and VARIA
  are pinned submodules. Imported ImGui, font and reference-pack notices remain
  intact. Local Git author identity is set for this repository only.
- **Presentation identity:** project/window title is The Zebes Project. Linux
  SDL game/editor icons reuse VARIA's original 16x16 Samus helmet, scaled with
  nearest neighbor. BMP V4 RGBA masks preserve transparency. Both icons are
  explicit NonUFS runtime dependencies. Development builds may retain Unreal's
  diagnostic title suffix; the internal module name stays SMUnreal.

## Review boundaries

Read the native generator bridge, save-slot switching, save-refill logic,
world/Animals/Mirror integration seams, packaging and project configuration.
No gameplay semantics changed in this review. Mirror and Race remain rejected
by the public settings/generator; guarded experimental Mirror code remains in
source, explicitly outside the supported feature set. Do not reactivate it.

The local Save Refill release and all player saves are untouched. Existing
Build/Package output has not been republished as a clean final release.
GitHub Actions and downloadable release artifacts are not configured here.

## Validation for this import

- SMUnreal Linux Development build succeeded after icon dependency changes.
  Build receipt contains both Splash/Icon.bmp and Splash/EdIcon.bmp as NonUFS.
- gaming-pc, isolated test directory: SDL2 decoded every RGBA pixel of both BMPs
  exactly; each 64x64 image has 1,520 transparent pixels.
- gaming-pc: Scripts/test-staging.py passed using synthetic inputs: default
  ROM exclusion, private opt-in, documentation allowlist, rejection of stale
  ROM/diagnostics and preservation of rejected output.
- Shell syntax checks passed for staging/build/package/play/asset preparation.
- Staged diff whitespace check passed, with imported vendor text preserved.
- Staged inventory: no ROM/save/native binary/archive; no file exceeds GitHub's
  ordinary file-size limit. Common credential-pattern scan found no matches;
  this is a targeted scan, not proof against every possible secret format.

No game was launched locally. The new window chrome has not been visually
accepted in a real desktop session, and Windows icon support is not claimed.
Historical randomizer solver/native tests are documented in their feature
checkpoints; they were not all repeated for this repository/branding change.
