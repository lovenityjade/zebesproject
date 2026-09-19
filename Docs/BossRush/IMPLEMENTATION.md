# Boss Rush — rules and implementation

Updated 2026-09-19. **Playable development implementation; player feedback remains necessary.**
This document supersedes the four-difficulty proposal in
[the minimum-equipment research](MINIMUM-EQUIPMENT-RESEARCH.md).

## Agreed difficulty direction

| Mode | Boss HP | Starting resources | Received damage | Drops |
| --- | --- | --- | --- | --- |
| Easy | **75%** of original HP | Normal baseline energy/ammo ×2 | Normal | Normal frequency, recovered quantity ×2 |
| Medium | Original HP | Baseline sufficient to win without drops | Normal | Normal |
| Hard | ×2 | Ammo budget adjusted for doubled HP | Normal | Normal |
| Very Hard | ×2 | Ammo budget adjusted for doubled HP | ×2 | Rare |
| Hardcore | ×2 | Ammo budget adjusted for doubled HP | **One damaging hit kills** | **None** |

The last Easy correction is authoritative: 25% less HP, **not half HP**.
Hard having no drops is superseded: Hard now has normal drops.

Implementation defaults retained from the latest proposal: rare means keeping
25% of otherwise successful drops; Hardcore inherits doubled HP. Both values
are centralized. Easy doubles quantities, not the number of on-screen objects.
“Resources ×2” never duplicates equipment upgrades. Hard/Very Hard ammo must
cover the larger damage pool, without making refill luck a prerequisite.
The baseline kits below provide a no-drop offensive route; human reaction time,
misses and preferred fight length still require player feedback.

Hardcore's exception is the Mother Brain survival cinematic: preserve the
Rainbow Beam spectacle and native Hyper Beam transfer, without scripted damage
killing Samus. Real combat attacks remain lethal. The exemption must be scoped
to that sequence, including its scripted follow-up damage, not all Mother Brain
attacks. No automatic reserves or other recovery may undo a Hardcore death.

## First implementation milestone

Implemented in `Native/sm_boss_rush.c` / `.h`:

- Central rules for five difficulties, with no dependency on VARIA or NG+.
- Ten-encounter catalog, including Mother Brain's three HP pools and flags for
  heat, underwater combat, positional defeat and scripted survival.
- 32-bit scaled HP and ammo budgets. MB3 at doubled HP is 72,000 and cannot be
  stored directly in the original 16-bit health field.
- Starting-resource and pickup-quantity calculations, saturating on overflow.
- Independent deterministic random stream for rare-drop filtering. It does
  not consume the native RNG that controls boss patterns.
- A run coordinator: idle → transition → combat → next transition; terminal
  failure/completion; full native-death acknowledgment required to advance.
- Per-encounter splits and gameplay tick accounting. Transition ticks do not
  count; paused ticks do not count. The adapter supplies native gameplay
  ticks rather than render frames.
- An input gate during transitions and the scripted Mother Brain interval.
- Post-suit damage policy, with the narrow Hardcore cinematic exemption.

The native mode preview now cycles **Easy → Medium → Hard → Very Hard →
Hardcore**, in both directions. Very Hard has a French label. Existing menu
automation was updated for the fifth entry.

The foundation-only milestone above is historical. The playable adapter below
now connects the menu, native fights, timer, drops and Unreal transitions.

## Remaining scope

- Human-played complete runs for equipment and difficulty balancing.
- Dedicated Boss Rush leaderboard payload/server integration; no run is uploaded
  or presented as online-ranked by this implementation.
- Optional roster extensions (the current sequence contains ten encounters).

## Special cases not hidden by the difficulty table

- Crocomire has a positional defeat condition. His living recoil distance is
  multiplied by 4/3 in Easy, 1 in Medium, and 1/2 in Hard/Very Hard/Hardcore.
  Fractional movement is retained. Forward movement, collision, hit reactions,
  weapon-specific recoil counts and the acid-fall defeat remain native.
  This scales positional resistance, not an invented health pool; elapsed
  fight time is not guaranteed to scale exactly with it.
- Bomb Torizo starts with Bombs collected; the native activation path is
  exercised by the combat probes.
- Kraid must remain reachable after standing up; no required wall jump on the
  accessible baseline unless deliberately selected later.
- Varia remains the baseline protection in heated rooms. A one-hit mode must
  not kill Samus automatically upon spawning in ambient heat.
- MB1's glass and approach obstacles cost ammunition independently of boss HP.
  The ammo helper is **not** a complete MB loadout generator.
- Very Hard doubles damage: its MB survival budget must cover a doubled drain
  if that scripted drain retains the damage multiplier. A Medium energy kit
  cannot simply be reused without checking that sequence.
- Mini-Kraid, Ceres Ridley and Metal Pirates remain optional roster candidates;
  the initial ten-encounter order is an implementation draft, not user canon.

## Verification

`Native/test_boss_rush.c` checks complete coordinator runs, rejected out-of-order
loads and duplicate death notifications, terminal death, timer/input barriers,
all damage-pool ammo budgets, 72,000-HP overflow safety, rare-drop replay and the
narrow cinematic exemption. Run the test executable and full native compilation
only in an isolated directory on **gaming-pc**. Actual verification evidence is
recorded alongside this document; no gameplay proof is implied by unit tests.

First pass completed on gaming-pc in
`projectSM/.tmp/boss-rush-foundation-20260918`:

- Rules/coordinator tests compiled with C17, `-Wall -Wextra -Werror`: passed.
- Full native library and randomizer host compilation: passed.
- Native menu input/render pass: all five entries, forward/backward wrapping,
  English/French, Start still locked, zero emulated CPU opcodes: passed.
- Very Hard label captures in both languages reviewed; text fits its row.
- Rare-drop replay retained 25,207 of 100,000 candidates with the fixture seed.
- Only fresh isolated SRAM was used; ROM hash unchanged. No local compilation,
  local game launch, personal save writes or release binary replacement.

See [machine-readable evidence](foundation-verification.json). Sanitizer linkage
was unavailable on this host (missing ASan/UBSan shared runtime); the strict
ordinary C build and tests passed. The modified Unreal automation source was
not rebuilt in this milestone. Menu captures come from the native renderer,
not a launched Unreal client.

## Playable arena integration — 2026-09-18

The native Start Game menu now starts Boss Rush. The session captures and retains
an in-memory native state/ROM snapshot, loads each of the ten original arenas,
and restores the menu on leaving. Vanilla/randomizer SRAM, run statistics and
route recordings must not be changed by an attempt. A generated seed cannot be
used as a Boss Rush arena base. Boss Rush does not create a persistent A/B/C slot.

Unreal owns the presentation handshake: source frame → detailed 4-pixel VR mesh
→ destination mesh → native combat. Inputs and combat time remain frozen until
Unreal acknowledges the reveal. The initial source is black; the final destination
after Mother Brain is black. No timed hold between normal combat transitions.

Samus is captured independently in both arenas. Her detailed wireframe segments
morph between the two silhouettes and screen positions, using the arena's
perspective plane and timing. A bounded directional stretch during relocation
returns to zero at both endpoints. The reveal uses the destination sprite,
including its actual suit, instead of sliding the previous sprite into place.

Death is intercepted before the native blackout: freeze the frame, dissolve the
arena to black, retain frontal Samus's wireframe for two seconds after the arena
has disappeared, then scatter its luminous segments. The existing Game Over
screen follows. Continue/End returns to the Boss Rush menu for a fresh attempt;
there is no mid-run retry.

The UI Debug page contains **Boss Rush practice** (invincibility + unlimited
ammunition), **Skip to next boss**, and **End Boss Rush test**. Practice only
changes Rush. Enabling it at any time irreversibly flags the current attempt as
unranked. It does not grant extra equipment. A new attempt resets eligibility
according to the toggle. No Boss Rush leaderboard upload is implemented yet.

Boss endurance is scaled through incoming damage with fractional carry, avoiding
16-bit overflow and preserving native health-dependent phases. Crocomire remains
a positional fight. Hardcore/Practice explicitly bypass Mother Brain's scripted
survival drain and the associated health gates, but keep the baby/Hyper Beam
sequence. Other difficulties retain the native survival phase.

Arena equipment is a first playable baseline, not yet a proven optimum. Charge
provides renewable damage for the long fights; heated arenas grant Varia,
Draygon grants Gravity. Test evidence must distinguish arena loading/debug skips
from actual native boss deaths and complete player victories.


## Arena kits (Medium baseline)

Energy and ammunition reset at each encounter; equipment is not carried forward.
Easy doubles resources. Hard/Very Hard/Hardcore double listed ammunition and
retain baseline energy, with their separate damage rules.

| Encounter | Equipment | Energy | Missiles | Supers |
| --- | --- | ---: | ---: | ---: |
| Bomb Torizo | Morph, Bombs | 99 | 10 | 0 |
| Spore Spawn | Charge | 199 | 10 | 0 |
| Kraid | Charge, Hi-Jump | 199 | 10 | 5 |
| Crocomire | Charge, Varia | 199 | 15 | 5 |
| Phantoon | Charge | 299 | 25 | 0 |
| Botwoon | Charge | 299 | 30 | 0 |
| Draygon | Charge, Gravity | 399 | 60 | 0 |
| Golden Torizo | Charge, Varia | 399 | 0 | 0 |
| Ridley | Charge, Varia | 499 | 0 | 30 |
| Mother Brain | Charge, Varia; native Hyper Beam later | 699 | 40 | 0 |

Mother Brain starts directly at the jar, excluding the Zebetite approach from
the ammo budget. Kraid gets Hi-Jump so his upper platforms do not require a wall
jump. Very Hard doubles the scripted Rainbow Beam drain; 699 energy is the full
starting budget, not protection against damage taken beforehand.

Spawn audit: Crocomire starts at (864, 128), clear of the low ceiling; the old
(864, 64) position overlapped solid tiles despite falling into the room after
combat resumed. Mother Brain starts at (220, 144), above the sloped floor so the
initial frontal hitbox is wholly in air. All ten hitboxes pass room bounds and
air-tile checks in READY and immediately after combat activation.

A Rush-scoped signed-coordinate correction keeps Draygon's death approach aimed
inside the arena after an off-screen fatal shot.

## Integrated verification

All builds and runtime checks run on gaming-pc in the marked disposable checkout
`.tmp/release-alpha025`, using fresh SMTests SRAM. The local client and personal
saves are not used. See runtime/combat/render verification JSON files alongside
this document. The original combat probe uses injected native shots and an acid-boundary
injection for Crocomire. The newer balance probe instead drives Crocomire
through actual charge-shot recoil to his first acid-fall state.
These are script/flow checks, not complete human victories or minimum-kit proof.

- [Runtime checks](runtime-verification.json): ten loads, transition freeze,
  sticky practice flag, Game Over handshake and unchanged test SRAM. Spawn
  hitboxes are checked against room dimensions and collision tile types before
  the reveal and at combat activation; movement is checked after loading.
- [Combat probes](combat-verification.json): all ten native defeat scripts,
  including Mother Brain's baby/Hyper Beam sequence.
- [Unreal rendering](unreal-verification.json): ten arena reveals, final blackout,
  Samus wireframe hold/explosion and Game Over; rendered frames inspected.
- The strict C17 rules test passed, including deterministic rare-drop sampling;
  actual native Samus damage was checked for all five difficulty settings.

The rendered integration check uses debug skips to advance arenas. It does not
prove normal player combat balance. The Debug controls compile and bind to the
tested practice APIs, but were not toggled manually in the rendered test.

## September 19 simulation polish — verified on gaming-pc

- Bomb Torizo starts with Morph Ball and Bombs; Bomb pickup bit 7 is already set.
- Crocomire receives Super Missiles. Its finite BG2 body is excluded from
  parallax, projected from its room position, and clipped before texture wrap.
- Encounter transitions begin at confirmed zero HP; Crocomire uses its initial
  acid-fall state, and Mother Brain requires the final post-baby phase.
- Failed: Continue repeats the current boss, retaining failed-attempt time;
  Retry resets the full run and timer; End returns to the native title.
- Continue counts are recorded separately; practice remains unranked.
  Pause offers Continue / Retry / End.
- Completed: native-font total time, difficulty, deaths, continues, ten splits.
- Only supplied 16:9 completed/failed art is used, letterboxed where needed.
- Subtle descending simulation data avoids the HUD and foreground sprites.
  Death disperses Samus's wireframe into a larger energy burst and fragments.

These changes supersede the earlier wait-for-complete-native-death handshake.
Native runtime, all ten injected defeat probes, and Unreal rendering/input checks
passed on gaming-pc. Crocomire was also moved offscreen through actual held-left
input: its body clips away instead of following the camera or wrapping. See
`crocomire-verification.json` and `simulation-unreal-verification.json`.
Continue was checked on Kraid, preserving both encounter and elapsed time;
Retry and End were checked through rendered menu inputs. Test SRAM was unchanged.
These automated checks do not replace human combat/balance testing.

The result screens now add sparks and additive electrical filaments around the
failed simulator. Completed sends lights along fourteen paths sampled from the
artwork's blue lines and sweeps a subtle reflection through Samus's green visor.
The original 16:9 artwork remains unchanged. Both supplied result recordings play
once, then stay silent until leaving the screen; see AUDIO.md.

Verified Linux binaries were copied locally after remote builds and tests, with
hashes checked against gaming-pc. Previous binaries remain in
`.tmp/boss-rush-polish/before/`. No local build or launch was performed.

## September 19 balance pass

The remaining Crocomire tuning is implemented in the generated A4 bank; the
upstream source is untouched. Both native rightward recoil instructions use the
same fixed-point scaling, scoped to the living Rush encounter. Ordinary play,
forward steps and death movement keep their original displacement. A fresh
arena or Retry/Continue clears fractional recoil state.

`Scripts/test-boss-rush-balance.py` covers all **50 encounter/difficulty pairs**:

- Actual native shot handlers receive ordinary missiles (100), supers (300),
  plain Charge (60), or the native Hyper Beam (1000). Enemy vulnerabilities and
  the runtime HP scaling remain active. Kraid's body/closed-mouth reaction is
  included, with aim following the current mouth box after he stands up.
- Successful finite shots fit the starting capacity. Mother Brain additionally
  reserves six missiles for the glass; its brain needs 23/30/60 damaging missiles
  in Easy/Medium/Hard and above, below the supplied 80/40/80 capacities. Charge
  supplies a renewable route for subsequent phases and other long fights.
- Twelve collision-aware Crocomire recoil instructions move him 64/48/24 pixels
  in Easy/Medium/Hard and above. Each full probe then pushes him to the acid
  through charge-shot reactions, without assigning a defeat position or HP.
- Mother Brain's mandatory sequence completes in every difficulty with practice
  disabled during that sequence. Hardcore retains health and receives Hyper
  Beam; normal combat still has the one-hit damage rule.
- Separate real Samus damage checks cover the five difficulties. The strict C17
  rules test also verifies pickup quantities, zero Hardcore drops, deterministic
  rare drops, and resource/HP calculations.

See [balance-verification.json](balance-verification.json). These are mechanical
viability checks with injected aim/timing/position and practice protection outside
Mother Brain's cinematic, not human victories or a proof of comfortable combat.
The kits need no additional equipment changes for the tested offensive routes.
Player feedback can still adjust pacing or forgiveness. The 100%/150% selector
and online leaderboard remain deferred at the user's request.

The verified native Linux library was installed locally after the remote checks;
its SHA-256 is recorded in the balance report. Previous binary:
`.tmp/boss-rush-balance/before/libsm_native.so`. No local game launch or build.

## September 19 Phantoon, native menus and death follow-up

Phantoon initializes a 4096-byte BG2 map to transparent tile 0x338, but the
original load uploads only its first 864 bytes. Arena snapshot loading left
opaque wall tile 0 in the rest of VRAM; scrolling Phantoon then scrolled that
rectangle with him. Rush now uploads the entire initialized map. His BG2 body
is also treated as finite, excluded from scenery parallax and streamed room
reconstruction. His native AI, vulnerability and shot handlers are unchanged.

Pause and Failed use the original ROM pause-frame tiles, map-arrow cursor,
palette and 8-pixel lettering. The translucent blue Unreal panel is removed
for those two menus. Continue/Retry/End retain their existing input and timer
semantics; the supplied Failed background and electrical effects remain.

The combat theme stops at the fatal hit. The existing Samus wireframe death
finishes completely before Failed: burst at 4.05 s, fragments gone by 5.40 s,
Failed at 5.60 s. Additive ribbons honor alpha during the fade. No additional
visual explosion is added. A procedural cybernetic impact with stereo echo
starts on the existing burst cue and ends after 1.30 s. It fires once per death.

Varia is equipped and collected before Crocomire, then retained. Gravity is
equipped and collected before Botwoon, then retained through Mother Brain.
These upgrades apply to all five difficulties and to Continue/Retry reloads.

Validation is performed only on gaming-pc. The active local game is preserved;
updated binaries are staged separately for its next requested restart.

Verified results for this follow-up:

- Native and Unreal builds passed on gaming-pc; all ten arena transitions and
  Continue/Pause/Retry/End inputs passed with audio enabled.
- Phantoon's transparent BG2 tail and preserved body passed the dedicated
  regression; frame 899 was visually inspected without the moving rectangle.
- English/French native menu sheets were rendered for all three selections.
  Unreal Pause and Failed screenshots were inspected. The current death
  capture `death-0120.png` is fully black before Failed; screenshots from
  previous runs must not be mistaken for the current run's logged frames.
- Fatal-hit silence, the impact's audible echo tail, no repeat and one-shot
  result music passed. Peak isolated impact PCM: 15938/32767.
- Gravity/Varia equipment checks passed across ten arenas. Mother Brain's
  scripted survival and Hyper Beam handoff passed again in all five
  difficulties with both suits. These use injected aim/position and practice
  outside the scripted sequence, not human victories.

Evidence: `phantoon-verification.json`, `death-audio-verification.json`,
`native-menu-unreal-verification.json`, `suit-survival-verification.json`.
Screenshots and audition WAV: `.tmp/rush-phantoon-native/`.
Verified binaries are staged under `.tmp/rush-phantoon-native/staged/` with
`SHA256.json`, using the same relative paths as the project. They have **not**
replaced the active local game's libraries. After the user requests a restart,
stop only its named unit, back up the current two libraries, install these staged
files with hash verification, then relaunch. No commit, push or release.

## Progressive beams and result entrances

The requested beam progression now replaces the original Charge-only kits:

| First encounter | Added beam |
| --- | --- |
| Spore Spawn | Charge |
| Kraid | Spazer |
| Crocomire | Ice |
| Phantoon | Wave |
| Golden Torizo (after Draygon) | Plasma |
| Mother Brain's native handoff | Hyper |

Earlier beams remain collected. Equipping Plasma disables Spazer; both remain
owned. Arena load, Retry and Continue derive the same kit from encounter order.
This is the chosen conventional progression for Rush, not a claim that vanilla
has a unique mandatory collection order. The test weapon dispatcher now reads
the equipped charged-beam damage from the original ROM table and passes the
actual beam type through native vulnerabilities.

Failed and Success use a one-second smooth fade from black for their entire
composition and from silence for their one-shot recording. The visual timer
resets before the first result frame, including repeated attempts. Music uses
the sample cursor, so playback speed and status polling cannot reset the fade.

## September 19 follow-up: late kits, Draygon and boss health

All changes in this section are **Boss Rush only**. Vanilla and Randomizer
inventory and encounter behavior are outside this change.

Draygon and subsequent encounters now include Morph Ball, Grapple and Power
Bombs (5 in Medium, 10 in the other difficulties). Golden Torizo receives 15
missiles and 5 supers in Medium, 30 missiles and 10 supers otherwise. Its beam
kit is Charge + Ice + Wave + Plasma, with Spazer collected but unequipped.
Continue/Retry use the same encounter loadout. The previous local executable
cannot acquire these changes without a restart with the updated libraries.

Draygon had two independent problems:

- The BG2 body map initialized 4096 bytes but uploaded only 1024, leaving an
  opaque repeated pattern. Rush uploads the complete map and clips the finite
  body in both axes; it no longer wraps onto the opposite edge of the screen.
- Native movement lost the shared `$0E26` swoop value before the return path,
  producing zero horizontal speed. Its left-boundary test also treated an
  unsigned 16-bit position as a positive 32-bit value. Rush preserves the
  scratch value and performs the signed 16-bit boundary test.

Three controlled movement cases now agree exactly with the original ROM
instructions, executed by a separate test-only CPU driver. The game runtime
still executes zero emulated CPU opcodes. Original intro state timing is kept;
the fixes do not speed up or skip the baby/entry sequence. Native rendered
frames through the intro, swoops and a grab were inspected. This is targeted
regression evidence, not a claim of a complete human fight validation.

The Boss Rush minimap is replaced by the original energy-tank tiles recolored
coral and an exact effective boss HP number. Ten tanks each represent 10% of
the current phase so large boss HP totals fit. Mother Brain follows her active
phase. Crocomire shows push progress and `PUSH`, since his defeat is positional.
Full and half-health captures were checked, including Golden Torizo.

Verification on gaming-pc:

- `progressive-beam-verification.json`: all 50 encounter/difficulty scenarios
  with actual beam damage/vulnerabilities and five Mother Brain handoffs.
- `loadout-verification.json`: all 50 real loaded kits, including the final
  Power Bomb/Grapple and Golden Torizo ammunition additions.
- `draygon-motion-verification.json`: three native-vs-ROM movement cases.
- `boss-hud-verification.json`: all ten HUDs and health changes.
- `result-fade-audio-verification.json` and
  `result-fade-unreal-verification.json`: music fade/one-shot checks and Unreal
  arena, transition, death and menu input automation. The latter predates the
  final Draygon/HUD native changes, which have their separate checks above.

The latest verified libraries are staged, not installed or launched:

```text
Native/build/libsm_native.so
22a013b75b349298a78ccf1da97f1d96f09e5622a799e7df56b913d0c87c6525
Unreal/Binaries/Linux/libUnrealEditor-SMUnreal.so
ad84de2aacb7e2cfd8c28d32932afe593564e650e6c917269698eee99d0a6468
```

These hashes match gaming-pc and `.tmp/rush-phantoon-native/staged/SHA256.json`.
The named local game unit was inactive at the final preparation check; no
automatic restart was performed.


## September 19 rendering optimization follow-up

The subsequent renderer batching and persistent quality settings are documented
in `RENDERING-PERFORMANCE.md`, with measurements in
`rendering-performance-verification.json`. This supersedes the staged-only
status above: the optimized libraries were installed while the local game was
closed, with backups and verified hashes in `rendering-installation.json`.
The game has not been automatically relaunched.
