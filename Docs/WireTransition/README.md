# Boss Rush wireframe transition — visual lab

Status: isolated Unreal visual study, **not enabled in normal gameplay**.

Approved direction: sweep a real room into luminous wireframe on black; morph
that geometry into the next room over 2.5 seconds, with no static holding phase;
sweep away the destination wireframe to reveal the next room. Samus retains the
original frontal elevator pose and stays visually fixed during the morph.
Sweep direction varies between screen edges.

`SMWireLab.cpp` renders native room snapshots and procedural, perspective-projected
architectural lines using Unreal Canvas. The source geometry comes from native
foreground/sprite provenance and visible palette boundaries in the native art.
The v2 grid uses **4 × 4 native-pixel squares**, compared with the first version's
16-pixel interior spacing: 16 times as many cells over the same surface.
Exact silhouettes are traced at pixel resolution; sustained color boundaries
add pipe, stone, enemy and armor details. Single-pixel interior dither noise is
filtered, while silhouette steps are preserved. Samus has visor, shoulder, chest,
cannon and leg detail instead of only a hollow outline.
It is a screen-space depth study, not a reconstruction of collision meshes.
For these isolated snapshots, the pose helper requests an effective-layer mask:
when native fog or backdrop covers the main screen, architectural provenance is
read from the subscreen. This leaves the actual pixels untouched. The request
is reset by native initialization and can only be enabled with an SMTests save.

Original background textures disappear entirely during the morph. Vertices and
sweep fronts have additive light; there is no strobing full-screen flash.

The lab preloads two real rooms (Crateria Keyhunter, Climb), extracts the
original frontal Samus sprite, preserves identical edges and matches other
parallel edges within 32 pixels, and records two opposite journeys. Spatial
buckets avoid a quadratic search as detail increases. The spatial sine-wave
warp has been removed: the entire mesh uses a common rigid perspective plane,
with straight parallel segments translating or shrinking/growing locally. The
light intensity is steady, without a traveling wave along the grid. A runtime
check verifies that interpolated primary segments remain axis-aligned. It deliberately does not implement asynchronous room
loading, a playable Boss Rush, item rules, rankings, or production input recovery.

All captures run on gaming-pc in an isolated checkout. `-SMWireLab` requires a
non-Shipping build and `ISOLATED_TEST_DIRECTORY`. Native pose preparation also
requires an `SMTests` save path. The lab copies the donor save through the normal
automation path. During the visual transition the HUD bypasses input/menu
handling and native simulation entirely; a full RAM CRC and native frame count
must remain unchanged. The process exits after recording and flushes held keys.

Reproduce on gaming-pc after compiling the native library and Unreal editor
module in the isolated checkout:

```sh
python3 Scripts/record-wire-transition.py "$PWD"
```

The recording script verifies unchanged donor SRAM and ROM hashes, native
simulation freeze, zero emulated opcodes, and 420 captured frames before encoding
14 seconds of silent 1200 × 672 / 30 fps video. A 120-tick GPU warmup is excluded.
Archive `Unreal/Saved/SMTests/WireTransition` before repeating a recording.

Production follow-up after artistic approval: bind a real room-loader barrier,
restore Samus at the destination's legal spawn, resume inputs on a fresh edge,
freeze both gameplay and run timing, and test abort/load-failure paths. Keep any
snapshot-derived wire geometry presentation-only: it must never alter collisions.

## Reviewed capture

[Watch the detailed v2 demo](Videos/Boss-Rush-Wireframe-Detail-v2.mp4).
[Previous version for comparison](Videos/Boss-Rush-Wireframe-Morph.mp4).
`verification-v2.json` records the current remote runtime checks and decoded-video
review; `verification.json` retains the first version.
The final recording contains two sweeps from different edges. Geometry morphs
for 2.5 seconds, with no static wireframe hold. Original and destination rooms
remain visible briefly outside the transition to make the comparison readable.
No local game process was launched and no production binaries were replaced.
