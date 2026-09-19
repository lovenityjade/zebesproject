# Boss Rush audio — 2026-09-19

The supplied **Boss Rush Theme – Face the Memory** replaces the original music
through arena loads, every fight and ordinary VR transitions. Result screens
use the supplied one-shot recordings described below. It loops continuously and starts over for a new
attempt. Leaving Boss Rush restores ordinary soundtrack behavior. Original
and Remastered preferences still apply to the other modes.

Native shots, impacts, enemy sounds and scripted sound effects retain their SPC
path. The load-from-save mute flag is cleared when the VR reveal hands control
to the player; previously it could suppress the first few seconds of effects.

## Supplied recordings and preparation

Sources, preserved unchanged at the project root:

- `Boss Rush Theme - Face the Memory.mp3` (239.600 seconds).
- `boss-rush-transition-tobefixed.mp3` (4.032 seconds).

Run `Scripts/prepare-boss-rush-audio.py` to prepare
`Soundtracks/BossRush/{Theme,WireIn,WireOut}.pcm` and its source/output SHA-256
manifest. This local optional music directory is already excluded from Git.
The player data directory is searched first, then `Content/BossRush`.

The theme targets **−21 LUFS**, leaving headroom for native effects. The
transition targets **−18 LUFS**. Boundary silence is removed at −45 dB: the
supplied transition's audible range ends at 3.258542 seconds. Pitch-preserving
time compression fits the 650 ms visual scan/reveal. Short endpoint fades
prevent clicks. The exit file is an exact time reversal of the processed
stereo frames, preserving left/right channels. Internal pauses are retained.

The music ducks smoothly by 6 dB during each sweep, then recovers. Global
volume and mute apply to both the native stream and Rush stream.

## Timing and implementation

`SMRushAudio.cpp` drives a separate procedural Unreal audio component. Its
real-time queue is independent of native simulation ticks, so loading, frozen
VR frames and future game-speed changes do not change musical tempo.
The native mixer streams bounded blocks from disk rather than loading the
whole recording into memory.

- At visual time 0: forward sweep, after the destination has finished loading.
- At 2.05 seconds: reverse sweep for the normal 650 ms reveal/final fade.
- During death: the reverse sweep instead begins at 4.05 seconds, matching
  Samus's wireframe explosion after her two-second hold.

Only music commands are intercepted; sample-bank uploads and SFX commands
remain active. Missing/invalid theme data falls back to the original audio.
Optional missing sweeps do not disable a valid theme or alter other modes'
soundtrack error messages. External cursors are not part of arena snapshots.

## Verification

Builds and runtime checks take place in the isolated checkout on **gaming-pc**,
with fresh `SMTests` SRAM. `test-boss-rush-audio.py` verifies real arm-cannon
sound output, playback with frozen simulation, arena continuity, a complete
music loop, missing-file fallback and unchanged SRAM. Captured native effects
and a combat mix are retained there as WAV files.

`test-boss-rush-unreal.py --audio` exercises ten arenas, final exit, a fresh
attempt and death with Unreal audio enabled. It checks all 13 forward/reverse
pairs and monotonically advancing music across the first attempt. This is
automated playback evidence, not a claim of human listening/balance approval.

Completed: native and Unreal builds passed; all eight native firing bursts
were audible; the full theme loop and fallback passed; all 13 Unreal sweep
pairs passed across ten arenas and a death test. The engine's actual output
was captured through an isolated audio sink. For this offscreen test only,
Unreal's unfocused volume multiplier was set to 1 (its default 0 otherwise
mutes an offscreen window). The temporary sink was removed afterward.
See `audio-native-verification.json`, `audio-unreal-verification.json` and
`audio-output-verification.json`. Verified Linux binaries were installed
locally for the next launch, with prior binaries backed up in
`.tmp/boss-rush-audio/before/`; no local build or game launch was performed.

## Earlier requests still pending

This audio change supersedes the earlier instruction to leave Rush music
alone. The September 19 simulation polish handles Crocomire's camera/body,
Super Missiles, and **Continue / Retry / End** pause/failure actions.
Paired 100%/150% difficulty entries and leaderboard speed categories remain
a separate pending request. See IMPLEMENTATION.md for current verification.

## Simulation result recordings — September 19

`simulation-failed-noloop.mp3` and `simulation-success-noloop.mp3` are retained
unchanged. `Scripts/prepare-boss-rush-results.py` produces `Failed.pcm` and
`Success.pcm` under the optional local `Soundtracks/BossRush` directory,
normalized to -21 LUFS with a -5 dBTP ceiling, without trimming or retiming.

On entering Failed or Completed, that recording replaces the combat theme and
plays once. The music bus stays silent after EOF for the rest of that visit.
Status polling, menu movement, and animated backgrounds cannot restart it.
Continue/Retry switches back to the combat theme; a later failure plays the
failure recording once again. End releases playback. Native sound effects and
the global volume/mute controls retain their existing behavior.

`test-boss-rush-result-audio.py` consumes both full recordings and checks silence
and a stable cursor after EOF, new attempts, repeat failures, and unchanged SRAM.
`test-boss-rush-unreal.py --audio --results` additionally holds both screens past
their track durations using the real Unreal audio component and captures their
animated presentation.

Both full-length EOF checks passed in the native mixer and in Unreal's actual
procedural audio component on gaming-pc. See `result-audio-verification.json` and
`result-unreal-verification.json`. A subsequent rendered regression confirmed the
additive electrical filaments, source-ink light paths, visor reflection, and menu
inputs. Frames from both animated result screens were inspected. The recordings
and verified Linux binaries are installed locally for the next requested launch.
This is automated playback/render evidence; music balance still awaits the
player's listening feedback.

## Death impact and silence

Fatal damage enters cue 5: the combat music stops immediately while the
presentation clock continues. The existing reverse sweep at 4.05 s also
triggers an original procedural cybernetic explosion, with no added visual
explosion. A descending modulated tone, filtered noise and low impact form
the attack; four diminishing stereo echoes finish within 1.30 s. The reverse
sweep is reduced by 6 dB during death to leave room for the impact.

The sound is synthesized once into a bounded buffer in
`Native/sm_rush_death_audio.inc`; no sampled Nintendo sound or new media file
is required. Duplicate burst cues cannot restart it during the same death.
Leaving the death state clears it. Global Rush volume/mute remains effective.
The native audio regression exports an isolated audition WAV and verifies
pre-burst silence, audible echo tail, no clipping, EOF silence and no replay.

## Result fade-in

Failed and Success each fade from silence to full music gain over one second
using a smoothstep envelope. The gain follows the actual sample cursor, so
status polling and menu movement cannot restart it. Playback still consumes
the full recording once and stays silent after EOF. The whole result screen
(art, effects and native labels) uses the same one-second curve from black.
The visual clock is cleared before entering the result state to prevent a
bright first frame after a previous result screen. Pause is unaffected.
