# Playtest features — September 16, 2026

## Saving resources

**Settings → Quality of Life → Refill energy and ammo when saving** controls the
optional refill. A randomized seed can also require `refill_before_save=on`.
Either enables it in ordinary play. Confirming **Yes** at a native save station
restores energy, reserve energy, Missiles, Super Missiles and Power Bombs before
writing the native save. **No** leaves everything unchanged. Both speedrun
categories force it off; the preference remains available for casual games.

The reported local incident was configuration: the local preference was false
and the active seed's setting was off. The personal preference is now true for
the next launch. The seed and its placements were preserved. Since September 17, the installation
default is on; existing explicit preferences are preserved. See the
[seed map and saved-station travel update](Tracker/SeedAtlas/README.md).

## Vanilla runs and New Game+

Save library offers Casual, Speedrun — No QoL and Speedrun — QoL when creating
a Vanilla bank. A completed Vanilla slot offers **New Game+ from SAMUS A/B/C**.
This creates a separate bank; the source SRAM and first completion snapshot are
preserved. Existing saves can unlock NG+ when completed with this build, but
cannot be retroactively certified as a full speedrun.

NG+ restores the equipment, equipped beams/items, current energy/ammo/reserves
and their capacities recorded at that first completion. Enemy endurance doubles
by halving incoming damage with fractional carry, avoiding 16-bit HP overflow
and retaining scripted boss phase thresholds. Enemy damage is multiplied by 1.5
before suit reduction. Animation speed stays unchanged.
Fractional damage is reset on enemy initialization/spawn, including a replacement
of the same enemy type in the same slot.
A fresh Start after an earlier attempt creates a new run ID, resets eligibility
and timers, and reapplies NG+ equipment; loading a saved run preserves its record.

Speedrun categories are independent for Vanilla and NG+. Glitches remain
allowed. No QoL forces the jump assists off; QoL allows only Wall Jump and Space
Jump assists. Save refills and trackers are suppressed in both. Debug teleport
and equipment grants invalidate the local result. The original in-game clock is
drawn under Energy with original ROM digits at native size. The system menu also
shows supplementary elapsed time while the process is running. Ranking data uses
**native in-game frames**, never that supplementary clock.

Metadata lives beside SRAM in `.runs`. A completed speedrun writes
`.run-<uuid>.json`. These are local claims, not verified online leaderboard scores.
The receiver foundation is documented in [Server/receiver](../Server/receiver/README.md).

## Chozo Tablet escape

Goals & Victory offers **3, 5, 6, 7 or 10 minutes** (default 5). The setting is
included in presets, saved requests, seed manifests and shared settings strings.
Older requests without it use 5 minutes.

After the required quota's item message closes, the original countdown starts,
Escape music replaces room music, and Unreal supplies a pulsing red wash and
explosions. The timer follows the native gameplay lifecycle, including Time Up.
A station save checkpoints the remaining time in `.relic-escape`; loading it
does not grant a new full timer. Boarding the ship stops the timer and follows
the original departure, ending and credits. This completion mode remains
incompatible with VARIA escape-route randomization and Scavenger mode.

## Run recap

Randomized gameplay records chronological position samples (up to ten per
second while moving) plus room/area changes and session gaps. Revisits are kept.
Replay uses the original ROM's area map tiles and palettes. It does not move
Samus, modify exploration, change inventory or step the simulation.

**System → Run recap** opens it; a completed recorded randomizer run also opens
it after the ending/credits. A toggles playback, Left/Right seek, L/R change speed,
and B/Start or Escape return. A history started on an older save is marked partial
when prior game time is detectable. Files are `.route-<slot>-<seed>` beside SRAM.
Starting the same seed again archives the previous route instead of overwriting it.
Copying a native save slot copies its source history; replacing or clearing a slot
archives the old destination history. Reusing the same seed in that slot therefore
does not merge unrelated runs. A rejected save-copy transaction leaves histories
untouched.

## Presentation

- Keep full signed sprite Y coordinates beside OAM, excluding wrapped fragments
  from the world render and the separately copied top HUD. The original game
  simulation remains intact.
- Hold the last coherent HUD through door-loading states, respecting fades.
- Replace the Morph Ball eye's native color-math beam in enhanced presentation
  with a soft, world-positioned engine light directed toward Samus.
- Space equipment and capacity groups more clearly in the wide map header and
  separate the area selector from check totals in the footer.
- Native save confirmation emits an engine light/spark event at Samus's station.
- Speed Booster/Shinespark add blue electric arcs and sparks around the original
  sprite. Enemy deaths during the boost and destroyed Speed Booster blocks emit
  a brief blue-white engine light; collision/damage remain native.
- Achievement unlocks queue a native-font toast and short ROM sound. Counters
  come from simulation events, independent of visual-effect settings. The
  Achievements screen lists the five existing milestones and progress.

## Evidence and limits

Tests use isolated saves on **gaming-pc**, not player SRAM. Results/captures for
this work are kept locally under `.tmp/playtest-polish`. Native game and Unreal
builds are distinct from runtime tests. These checks do not constitute a complete
playthrough of every room, boss or generated seed.

- Real station interaction: option off, cancellation, option on; Vanilla and
  Randomized; resource values and reload persistence; original ROM hash unchanged.
- Both speedrun categories: native station refill blocked; correct assist policy.
- NG+: completion import, exact snapshot, damage scaling, load persistence,
  respawn damage carry reset, fresh-attempt identity/equipment, in-game timer,
  completed JSON and unchanged source SRAM.
- Chozo escape: every duration, quota, saved remaining time, native Time Up,
  ship departure → ending → credits and completed route record.
- Original eye HDMA active in both presentation modes: identical simulation RAM.
- Native tracker regression: 52 assertions/cases reported, including item header
  and original map/minimap colors. Signed sprite-coordinate boundary regression.
- Real door transitions: coherent HUD and equal movement/timing with wide off/on.
- Read-only native-map route replay after movement and revisits.
- Managed route histories: accepted/rejected slot copy, destination archive,
  clear and reuse with the same seed; source history unchanged.
- Unreal recap: automatic opening at the ending's final state, play/pause, seek,
  speed, close without reopening; unchanged simulation RAM and rendered capture.
- Real Samus placed below a one-screen room: top HUD stays unchanged in 4:3 and
  widescreen at three vertical boundary positions.
- Unreal captures: real station save glow/toast, native pause, Achievements
  screen, recap, and charged Speed Booster lighting/particles.
- Speed Booster: ordinary native running exercises real charging and active
  contact state. Its separate Unreal capture holds a charged contact state to
  inspect the light/sparks; it is a presentation fixture, not an end-to-end
  traversal of the collapsing-floor corridor.
- Receiver: database validation, all four categories, pending-only ingestion,
  authenticated live HTTP, duplicate/conflict handling, restart and health.

Repeatable follow-up scripts: `test-route-slot-history.py`,
`test-playtest-runs-route.py`, `test-sprite-hud-boundary.py`,
`test-transition-hud.py`, `test-speed-effects.py`, and `test-playtest-unreal.py`
under `Scripts/`. They require the marked isolated gaming-pc test root and never
use the player's SRAM. The native charging test and renderer fixture deliberately
cover different boundaries; do not report the renderer fixture as a complete
natural run-up through Norfair's post-pickup escape state.

The existing distribution audit still blocks packaging pending earlier embedded
asset/provenance work. This task does not bypass that gate.
