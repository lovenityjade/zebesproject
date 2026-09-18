# Varia and Gravity suit transformations

The presentation replaces the original 256-pixel HDMA light beam with an Unreal
energy build-up, orbiting particles, electric filaments, a full gameplay-view
flash and expanding shockwaves. The original Samus sprites, poses and suit
palettes remain in use. Varia uses gold/cyan energy; Gravity uses violet/cyan.

## Synchronization and rendering

`sm_visual_state(49)` reads the active suit HDMA object (Varia `88:E026`, Gravity
`88:E05C`); field 50 exposes its native stage. It does not write simulation state.
The discharge starts on stage 4, immediately after the native stage 3 routine
awards the item and calls `Samus_LoadSuitPalette`. The presentation never grants
or substitutes equipment. Its clock advances once per native simulation frame,
so pausing or a slow render does not move the effect ahead of the acquisition.

The SNES beam uses window masks and color math configured for a 256-pixel image.
`sm_scene_replace_suit` removes these on presentation PPU **copies**, including
widescreen spans. Both window representations used by the reference and batch
renderers are cleared. The reference frame, HDMA, collision and item logic remain
unchanged. Disabling Atmosphere restores the original native beam.

Unreal projects procedural XYZ particle paths and rotating rings in perspective
onto the game canvas. This is an additive presentation effect, not a replacement
3D Samus model. Soft additive ribbons are used instead of Canvas line primitives,
whose blending produced opaque, dark outlines during the first visual check.
No generated artwork or new Nintendo-derived asset is required.

The existing **Flash intensity** setting controls the screen flash, lightning,
rings and strong glow. At zero, only subdued particles and gradual dimming remain.
The HUD stays outside the gameplay flash. Acquisition achievements are evaluated
after the native transformation so their notification does not cover it.

## Controlled recording

Build and run on **gaming-pc**, maximum two build workers. Do not build or run the
local player's copy. The test root must contain `ISOLATED_TEST_DIRECTORY`.

Development-only `-SMSuitTest=1` or `-SMSuitTest=2` copies the SRAM provided by
`-SMTestSave=...` into `Saved/SMTests`. It loads the real Varia (`A6E2`) or Gravity
(`CE40`) room, marks the relevant boss cleared in the fixture, prepares the prior
suit and invokes the game's actual acquisition routine. This bypasses collecting
the room pickup and is not a complete playthrough.

The fixture captures 360 actual Unreal viewport frames at 1280 × 720, advancing
exactly one native frame (`736 / 44100` seconds) per image. Native stereo audio
is saved alongside them. Achievement notifications are suppressed only in this
capture fixture. `SM_SUIT_CAPTURE_COMPLETE passed=1` checks the resulting item,
returned gameplay state, HDMA completion and zero emulated CPU instructions.

Example invocation (paths are isolated test paths):

```sh
UnrealEditor SMUnreal.uproject /Engine/Maps/Entry -game -windowed \
  -ResX=1280 -ResY=720 -nosplash -unattended -vulkan \
  -SMSuitTest=1 -SMImageScaling=0 -SMTestSave=/path/to/copied-fixture.sram
```

`Scripts/test-suit-transformation.py ROOT LABEL SUIT ENHANCED` also runs the native
sequence and records per-frame hashes of all simulation RAM and original PPU
pixels. It uses the existing isolated boot helper from
`test-motherbrain-wide.py`. Comparing enhanced and original runs verifies that
the presentation does not alter the native sequence.

Evidence and videos: `Build/SuitTransformation/` (private local artifacts).
No release upload or publication is part of this change.

## Verified results — 2026-09-16 (America/Toronto)

- Native C and Unreal Editor Development builds succeeded on gaming-pc.
- Varia and Gravity: 420 native frames each, three configurations (original,
  enhanced batch decoder, enhanced reference decoder). All simulation RAM and
  original PPU frame hashes match across configurations. The enhanced batch and
  reference presentation samples also match.
- Both native acquisitions reach stage 4 on frame 65 and finish on frame 164.
  From stage 4 onward, and after completion, the complete 16-color Samus palette
  matches the appropriate original ROM palette.
- Both 360-frame Unreal/Vulkan captures finish in gameplay with the correct suit
  flags, no active pickup HDMA and zero emulated CPU instructions. Key frames
  were visually inspected: charge, discharge and completed orange/purple armor.
- ROM and source SRAM hashes remain unchanged. Tests write only isolated saves.

A first capture exposed a fixture issue: the room-preview loader starts Zebes's
arrival palette animation (`8D:E1F4`), which could overwrite the acquired suit
palette if the acquisition was triggered too soon. The fixture now waits for
arrival to finish before recording. No gameplay palette workaround was added.

Final MP4s: H.264/AAC, 1280 × 720, 360 frames at the native simulation cadence
(about 59.92 fps), 6.008 seconds each. Both stereo tracks contain game audio.
`video-verification.json` records ffprobe metadata and SHA-256 checksums; local
copies were hash-checked after transfer from gaming-pc.
