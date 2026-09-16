# Native startup and layered title presentation

Current presentation release: **ALPHA-0.24**. See the
[validation record](ALPHA-0.24-VALIDATION.md) for the revised notices and fades.

The game starts with two English notices, followed by the creator logo, **2026**,
and the approved title composition. The notices appear once per application
launch, after the mandatory ROM gate. Resetting the game keeps the native intro.

Each notice lasts ten seconds or until a fresh keyboard/controller/mouse press.
A held input cannot dismiss both notices. No native frames or music are consumed
while either notice is visible; the last dismissal is consumed before play resumes.

In ALPHA-0.24 both notices use a pure black background and a Slate-rasterized
Montserrat font at viewport size, rather than an enlarged bitmap font. The
timeout remains ten seconds but has no visible timer or progress bar. The only
footer is “Press any button to continue”.

## Notices

**PHOTOSENSITIVITY WARNING**

This game contains flashing lights, bright visual effects, and rapidly changing
images. These may trigger seizures or other symptoms in people with photosensitive
epilepsy, including those with no previous history of seizures.

Play in a well-lit room and take regular breaks. Stop playing immediately if you
experience dizziness, visual disturbances, involuntary movements, or discomfort.

**AI TRANSPARENCY**

We believe in being open about how The Zebes Project is made. The voice performances
and music are created and performed by real people. The game's code is developed
by humans with AI assistance.

Some graphics in this development version were generated using AI as temporary
placeholders. These assets are not intended to be final and will be replaced with
human-created artwork.

## Native timing

`SMTitle.cpp` reads existing native state/cinematic function pointers after each
emulated frame. It replaces the presentation, not the cinematic interpreter,
audio queues or input logic. Timing below is relative to the first native frame,
after the notices. A native frame is 736 / 44100 seconds.

| Native frame | Presentation |
| --- | --- |
| 1 | TheLovenityJade logo, original brightness fade |
| 151 | 2026 |
| 281 | Samus / left-side pan |
| 597 | Upper boss pan |
| 845 | Lower boss pan |
| 1245 | Pull back into full composition |
| 1625 | Project logo reveal |
| 1689 | Press Start |

Native black intervals, skipping the cinematic, save selection, attract-mode demos
and music scheduling are retained. Title effects use the existing Atmosphere and
Flash Strength controls. The composition has separate 16:9 and 4:3 layouts.

The three opening pans fade in and out over 0.45 seconds within their existing
native cue windows. The full composition fades in over 0.55 seconds. Native
black holds and music/frame scheduling are unchanged. The year uses the original
two-tile-high menu digits decoded from the validated ROM into a transient texture.

The front end displays **ALPHA-0.24** in its lower-right corner, including notices,
title and native save/options screens. Root `VERSION` supplies the build label;
UnrealBuildTool tracks it as an external dependency. For each subsequent release,
update that file, `ProjectVersion` in `Unreal/Config/DefaultGame.ini`, and
`CHANGELOG.md`, then rebuild both Game and Editor targets.

## Artwork

The approved layered browser composition lives in `Labs/TitleScreen`. The native
renderer uses the same cropped PNGs and placements, including Samus with two hands
and the stacked bosses. Original images are not repainted by the renderer.
Depth-dependent camera motion, twinkling stars, luminous eyes, dust, fog and logo
glow are rendered separately in Unreal.

Run `Scripts/prepare-title-assets.py` on gaming-pc to stage the reviewed layers in
`Unreal/Content/Title`. It checks each source SHA-256 against the lab manifest and
copies eighteen PNGs plus crop metadata. Runtime dependencies include this directory.
The Nintendo font is deliberately excluded: the validated user ROM supplies the
alphabet/digits into a transient texture at runtime. Existing distribution gates
remain in place.

## Validation

`Scripts/test-title-presentation.py ISOLATED_ROOT` runs four rendered scenarios on
gaming-pc: automatic notice timeouts, held-input dismissal, skipped cinematic and
4:3 composition. It asserts native frame zero during notices, the original cue
frames, arrival at save selection and captures all presentation stages. The
isolated root must contain `ISOLATED_TEST_DIRECTORY`; each run uses temporary SRAM.
Set `SM_TITLE_EDITOR` to the UnrealEditor executable to validate the actual
`UnrealEditor -game` launch path too. Logs, screenshots and `verification.json` are
stored under `ISOLATED_ROOT/title-results`.

Set `SM_TITLE_HEADLESS=1` with `SM_TITLE_EDITOR` for a module/startup/timing check
without rendering. Such a result explicitly records `rendered: false` and must
not be described as an Editor visual check.

### September 16 integration validation

- Development Game and Editor builds passed on gaming-pc (two compile actions).
- All four Game scenarios passed with Vulkan rendering: 11 captures each for
  automatic, held input and 4:3; 7 captures for the skipped intro.
- Automatic notices measured approximately 10.00 seconds each, with native frame
  remaining zero. All eight no-input cinematic cues matched the native baseline.
- Render review caught and corrected additive halo rectangles, camera overscan and
  low-contrast lettering over the planet. Corrected 16:9/4:3 captures were inspected.
- The first Editor Vulkan launch exceeded its 240-second startup budget while
  compiling global shaders, before gameplay. Its log is retained separately;
  rendered Editor verification is not claimed.
- Editor `-game -nullrhi` passed the complete automatic startup, notices, native
  cue assertions and arrival at save selection. This check produced no images.
- Both matching binaries and the eighteen title layers were installed locally;
  executable/module hashes matched gaming-pc. Previous binaries are retained in
  `.tmp/title-native/before-install`. Logs/captures are in `.tmp/title-native`.
  Player saves and the native core library were not changed by this integration.
