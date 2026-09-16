# ALPHA-0.24 validation

Scope: native front-end mode selection, slot emblems, startup typography, year,
opening fades and build label. Story and Boss Rush are unavailable previews;
this release does not implement their gameplay or create their save types.

All compilation, automated execution and rendered checks run on gaming-pc under
`/tmp/sm-native-generation-20260914`, marked `ISOLATED_TEST_DIRECTORY`.
Player profiles and SRAM are not used by these tests.

- `test-native-slots.py`: mixed Vanilla/Randomizer banks, independent seeds,
  generation commit, pending/ready/error gating, copy/clear and write-failure
  protection passed. Uses existing validated placements, not new seed generation.
- `test-title-menus-native.py`: natural intro and file-select navigation,
  copy/clear, all four modes, all Boss Rush difficulty previews and generation
  states passed. Native pixels equal the composed protected GUI and scene.
- `test-mode-presentation.py`: Vulkan captures of saves, Vanilla, Story, Boss
  Rush Easy/Hardcore and pending Randomizer. Layouts visually reviewed.
- `test-menu-flow.py flow`: native mode selection through the real Unreal profile
  callback, defaults, selected-slot settings handoff, persistence and B/C
  isolation passed in Game. Editor validation is recorded separately below.
- `test-title-presentation.py`: automatic 10-second notice timeouts, held input,
  skipped intro and 4:3 composition passed. Fourteen captures per full intro,
  eight for skipped intro. All eight native cue frames match the baseline.

The first visual run caught a silent Canvas rejection of filename-only Slate
fonts. The fix uses a retained runtime UFont backed by Montserrat. Capture checks
now require actual pixels for both warning paragraphs and the build label.
The menu render fixture also now finishes native boot initialization before
entering file selection; production boot behavior was unchanged.

Local evidence is in `.tmp/alpha024/`; original startup integration evidence
remains in `.tmp/title-native/`. Build success and these focused checks do not
claim a complete gameplay playthrough or exhaustive randomizer validation.

Both Development targets built successfully. The Editor module passed the same
native/profile flow with `-game -nullrhi`; this is not an Editor rendered check.
The final Game build passed all six Vulkan menu captures. Additional checks of
the saved intro images confirmed both warning bodies, black backgrounds, native
year digits and a visible version in every front-end stage. Fade captures have
lower brightness than the fully visible shot and reach black between shots.

Matching Game, Editor and native-core binaries were installed locally after
SHA-256 comparison against gaming-pc. Hashes are recorded in
`.tmp/alpha024/build-hashes.json`; previous files are preserved under
`.tmp/alpha024/before-install`. No local game was launched during this update.
