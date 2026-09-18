# Mother Brain body repeating in widescreen

Reported September 16, 2026: Mother Brain's body appears a second time on the
right edge during the final battle.

## Cause and correction

`MotherBrainBody_FakeDeath_Ascent_6_SetupPhase2Gfx` (A9:8D11) changes BG2 into a
256-pixel-wide body tilemap. SNES background sampling repeats that tilemap;
the extended viewport exposed the next copy. The brain/neck use a different
drawing path, explaining the headless duplicate.

`Native/sm_scene.c` recognizes this BG2 mode only in room DD58, after both BG2
scroll flags become 1. `Native/sm_wide.inc` recovers the signed ten-bit scroll
and excludes repeated copies when composing the extended spans. It recomposes
the other layers instead of painting over the final image. The original body
texture can still extend into the margins. Body BG2 is excluded from additional
presentation parallax. Ordinary repeating backgrounds and phase 1 are untouched.

The original PPU, room geometry, collisions, simulation, and saves are unchanged.
This is scoped to Mother Brain; other bosses need their own rendering evidence
before applying similar treatment.

## Validation

Built the published ALPHA-0.24 source with only these two source-file changes on
`gaming-pc`, at `/tmp/zebes-motherbrain-wide-fix`, using two build workers. No local
build and no changes to the player's installation or saves. Paused ending work
was excluded.

`Scripts/test-motherbrain-wide.py` loads an isolated copy of the reference save,
enters the actual room and runs the native ascent and phase-2 routines. It bypasses
the initial fight for setup. Run it with the original library as `baseline`, then
with the corrected library as `fixed`, `fixed-reference --reference-renderer`, and
`fixed-parallax --parallax`.

Twenty-one frame comparisons passed:

- Ordinary room background and phase 1: identical pixels.
- Ascent/phase 2: duplicate removed in the frames that exposed it.
- Native simulation RAM and original PPU pixels: identical before/after.
- Original central gameplay image: identical before/after.
- Fast renderer, reference renderer, and forced extra parallax: identical corrected images.
- Source ROM and reference save hashes: unchanged; zero emulated CPU opcodes.

Before/after captures were visually inspected. Private evidence is under
`Build/MotherBrainWide/`, including `verification.json` and
`baseline/phase2-1000.png` versus `fixed/phase2-1000.png`.

This validates native rendering, not a complete manual battle or a new packaged
Windows/Linux release. The existing GitHub downloads have not been replaced.
