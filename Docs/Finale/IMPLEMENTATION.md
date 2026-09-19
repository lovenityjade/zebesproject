# Mother Brain finale

Scope: Vanilla, Randomizer, Boss Rush. Presentation follows the original native
scripts; boss AI, damage, collisions, Hyper Beam grant, and escape remain native.

## Presentation

- ROM-derived baby sprite pulsates while draining Mother Brain. Green energy
  motes converge into its core; restrained translucent saliva trails fall below.
- Last charge gets an eye charge and bright directed impact beam.
- Sacrifice freezes a reconstructed room frame in grayscale. Samus and the baby
  retain color. The baby dissolves into luminous droplets converging on Samus,
  followed by an energy aura and small electrical arcs.
- The baby's native fall still runs invisibly: its Y position gates completion
  of the original palette fade. Removing that simulation would softlock the
  encounter. No additional enemy or item is created.
- Return to the duel restores color and plays **Face the Memory at 0:00**, with a
  two-second fade-in. This uses separate cue 6; ordinary Boss Rush uses cue 0
  from the beginning. Only music is muted during sacrifice; SFX stay native.
- Hyper Beam has a rainbow ribbon, bright muzzle, restrained visual recoil and
  small crimson droplets/flesh fragments on actual native beam reactions.
- Final death adds textured pixel fragments and surface dissolution. Face the Memory continues through escape; the native Escape cue is suppressed. Ordinary Boss Rush keeps its separate result handoff.

## Implementation boundaries

`Native/prepare_finale.py` adds guarded hooks to generated banks, without editing
upstream. `sm_finale.c` owns the phase/event observations. `sm_finale_scene.inc`
reconstructs the exact background behind Samus/baby OBJ texels on a presentation
PPU, restoring the original fields immediately. `SMFinale.cpp` composes the
original sprites and draws bounded effects in Unreal. Ribbons and particles use
one triangle batch, not an individual rotated Canvas draw for each line.

Atmosphere controls the enhancement. Flash strength limits added illumination;
Low rendering quality lowers particle counts. Disabling enhancement preserves
the native baby sequence and original music. Temporary profiles use an explicit
`-SMTemporarySave=` path containing `/SMTests/`; they never select the usual save.

## Validation

See the adjacent verification files for the final verified state. Native test
setup bypasses the tube fight and protects test Samus during that setup; its
script timeline is not a human completion claim. Render checkpoints and the
separate pre-door save require their own validation.

### Verified on gaming-pc

- Native timeline: drain → healing → final charge → sacrifice → transfer →
  Hyper duel → death → escape. Hyper Beam granted by the original native script;
  zero emulated CPU opcodes. The isolated setup protects Samus from its artificial
  arena entry, so this is progression evidence rather than a player victory.
- Boss Rush: Mother Brain defeated on all five difficulties with enhancement
  enabled; native weapon damage and scripted survival verified. Aim/timing are
  injected by the automation, not human victories.
- Two rendered Unreal passes: complete sequence, then aimed Hyper duel. The duel
  registered 23 real beam impacts / 24 shots. Screenshots reviewed for selective
  grayscale, removal of the baby's sprite, colored droplets/aura and rainbow beam.
- Audio test: silence during sacrifice, first sample position exactly zero,
  two-second fade, cursor retained over pause, ordinary Rush still starts at zero.
- Separate SRAM: native station 0 in Tourian (room DE23), all ordinary equipment,
  living Mother Brain, no Hyper Beam. Load and actual door entry verified with
  native movement and five missiles to open the red entrance door.

### Local test save

`Scripts/play-finale.sh` selects `.tmp/SMTests/MotherBrain/sram.dat`, separate from
normal profiles. Choose slot A. Leave the station to the right, descend to the
bottom of the Rinka shaft, then open the red door on the left with missiles.
The encounter starts through its original door and tube-fight sequence.

The save and screenshots are local test artifacts, not release assets. Current
validation is Linux/gaming-pc, not a Windows package or a complete human playthrough.

### Temporary-session profile correction

The first interactive launch incorrectly restored the last active randomizer
bank after loading the fixture SRAM. The earlier native save validation did
not exercise this menu boot path. `SMTemporarySave` now isolates presentation
settings, achievements and the profile root beside the fixture, ignores the
last active bank, and imports the supplied SRAM into a fresh Vanilla bank.
The normal legacy save is excluded from the temporary profile picker.

Regression check uses real Unreal menu initialization without `SMTest`, verifies
all three slot manifests are Vanilla and the bank SRAM equals the fixture,
and hashes the normal SM directory before and after to detect unwanted writes.

The current pixel-texture revision, visual fixes and escape/audio verification are documented in `PixelV2/IMPLEMENTATION.md`. Its reports supersede the original 1:14 audio and smooth-ribbon visual baseline.
