# Game Over presentation

Authorized scope: replace only the Game Over presentation using the supplied
`gameover-16:9.png`, `gameover-4:3.png` and `gameover.mp3`. Other playtest backlog
items remain on hold.

## Presentation

- Select the supplied image matching the widescreen setting, with the existing
  Fit / Integer Scale policy. Preserve its aspect ratio and complete composition.
- Replace all previous Game Over graphics. Draw CONTINUE and END centered below
  GAME OVER using the native font read from the player's validated ROM.
- Keep the original Up / Down / Select navigation and A confirmation. The selected
  choice is bright; the other is dim. Continue returns to the saved game's map;
  End returns to the title screen.
- Flicker the spotlight through a soft runtime mask. Keep the title and choices
  steady. Draw 160 small drifting dust particles, brighter within the light cone.
- Preserve native fade transitions and stop the custom presentation on exit.

`Unreal/Source/SMUnreal/SMGameOver.cpp` owns the rendering.
`Native/sm_gameover.c` exposes menu state and renders labels from the ROM font.
No exported Nintendo font image is added to the repository.

## Assets and music

Run `python3 Scripts/prepare-gameover-assets.py` to reproduce runtime files in
`Unreal/Content/GameOver/`. PNG copies are byte-for-byte identical to the supplied
files; `sources.json` records hashes. The MP3 is decoded by ffmpeg to 44.1 kHz
stereo signed 16-bit PCM with an MSU1 header, looping from the beginning.
Original files are preserved. NonUFS runtime dependencies stage both images and
GameOver.pcm with Unreal builds.

In `Native/sm_soundtrack.c`, replace only bank 3 / command 4 while the game is in
state 26. Keep the original sound-bank upload and Metroid sound-effect commands.
The replacement works with both Original and Remastered soundtrack settings and
uses the existing master volume/mute controls. Stop it on Continue, End or core
shutdown. If the file cannot be opened, retain the native audio fallback.
The Unreal audio queue is cleared on entry/exit to avoid stale queued music.

## Native End correction

The upstream comparison runner handles `game_state == 0xffff` by resuming the
native reset coroutine at phase 3. The port's native-only runner bypassed that
hook, so End could dispatch an invalid state and crash. `sm_bridge.c` now performs
the same coroutine handoff after the frame, preserving SRAM and executing no
emulated CPU instructions. Upstream source remains unchanged.

## Validation

Tests run only in the marked isolated directory on gaming-pc, with copied saves.
`Scripts/test-gameover-native.py` exercises Continue and End for both soundtrack
settings, checks label changes, custom music start/stop and zero emulated CPU
opcodes. A silent replacement PCM isolates the remaining native Metroid audio.
The source ROM hash is verified unchanged.

The non-shipping `-SMGameOverTest` fixture additionally requires the isolated test
marker. It captures lit and flicker frames then exits. Add `-SMGameOverClassic`
to exercise the 4:3 artwork. The native test trigger requires an SMTests save path.

Validation results: native Continue/End passed in both audio modes; isolated
Metroid audio passed; all 32 existing music queue cues, soundtrack switching,
missing-pack fallback and credit-preview restoration passed. Game and Editor
builds compile successfully. Render captures are inspected in both aspect ratios.
