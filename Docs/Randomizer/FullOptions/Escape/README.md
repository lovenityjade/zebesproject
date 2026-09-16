# Escape and Disabled Tourian — public integration checkpoint

The full integration goal remains active. Public `escapeRando` and Disabled
Tourian requests are now enabled. Disabled Tourian forces escape on and retains
enemies. Chozo Relic Hunt keeps its separate ending and rejects timed escape.
The candidate is isolated at `.tmp/escape-native` and gaming-pc
`/tmp/sm-native-generation-20260914`; no local launch or package/release replacement.

Implemented common behavior follows pinned `rando_escape_common.asm` and
`objectives.asm`, translated to native C:

- A 56-byte clock sub-contract with independent pending/applied slot state and
  validated BCD static/regional values. `sm_escape.capture_clock` calls the
  actual source `applyEscapeAttributes` writer and retains its skill-scaled,
  rounded full/half clocks. No executable IPS bytes are installed.
- Objective quota triggers escape only for the Disabled-Tourian contract.
  The `nothing` objective waits for graph region Crateria where required.
  Triggering opens the source 32 door bytes, marks boss bits/tube/all-objectives,
  loads timer graphics, starts native timer state 2 and original escape music.
  It preserves acquired equipment: Plasma is not granted Hyper Beam behavior.
- Original MB timer routine reads either the static BCD time or ten-region
  source table; original timer decrement/display routines remain in use.
- Source extended-room filtering, two/three-pixel shaking and explosion helper,
  timer graphics refresh and escape music preservation after boss drops.
- Native map stations grant all area maps with escape randomization; save
  stations follow their refusal branch during escape. Original green-gate,
  bomb/PB/super-block reactions allow Hyper Beam only in post-MB escape.
- The original MB escape door initializes common escape state. Enemy removal
  retains 15 source special-room populations, including six exact elevator
  records (114 data bytes). Disabled Tourian retains enemies. Returning to an
  unconfigured plan restores all authored population bytes.

`Proofs/Capture` records three source generations (15093500..02): randomized
post-MB escape, Disabled Tourian, and Disabled Tourian with Minimizer. These are
capture fixtures, not public successful native generations. Their clocks are
consumed by the native tests: 30 native timer starts, 48 enemy-header cases,
objective triggering, refused native save PLM, PB block behavior, BCD rejection,
restoration and zero interpreted CPU opcodes pass. Native compilation passes.
That earlier common-behavior proof did not cover Unreal or a full escape playthrough.

The ordered-routing layer is now implemented (catalog
`52ab6049991b71e1b32991ee2c99a3516e8de1d915c6d22b908cfd6bc1de9f18`).
`build-escape-routing.py` derives 289 descriptors for 17 endpoints through the
actual pinned graph writer. The 44-byte routing contract retains repeated
source writes in order. Native arrivals use original callbacks and advance the
four-step Flyway cycle only during escape. Original room setup uses the authored
Flyway/animal return lists, animal-room shotblocks, WS scroll fix and grey-map
door variant. Reviewed room/table spans are data only; old descriptors and spans
are restored before applying another slot.

`Proofs/routing-native-results.json` reuses the existing three source captures,
without regenerating unrelated seeds. All 31 effective physical descriptors and
native arrival/scroll/spawn callbacks match those captured writes, 24 animal
cycle exits pass, A/B/C switching and invalid-update rejection pass, and clearing
the plans restores original data. It also exercises original room setup and PLM
routines. This proves the routing sub-contract, not public generation or a full
collision-driven escape playthrough. The isolated candidate's exact hash is in
the proof; the local package and frozen release remain untouched.

Production binding now includes schema-5 objectives (same 272-byte ABI), typed
Unreal clock/routing plans, per-slot configuration/clearing and native generation
commit. Historical objective schemas remain supported. The fresh solver and
embedded tracker use the same escaped-world topology; Disabled Tourian completes
at the ship after the goal quota, without Mother Brain. The original objective
pause displays `Tourian: Disabled`.

The remaining authored WS patch is audited and applied as compressed level data:
state CB22 references C4:BDC0, whose first 19 compressed bytes are unchanged. The
new stream expands to the same 36,866 bytes; only four block-type bytes and four
BTS bytes differ. Seven sealed map-station tiles are blanked and excluded from
both map totals and exploration counts, including retained Minimizer regions.

Evidence for this increment:

- `Proofs/Public`: three public generations, 15093500..02, with 100/100/67 checks.
  Every retained check and the configured ending passes the fresh source solver.
  Requests deliberately ask to remove enemies/disable escape with Disabled
  Tourian; effective dependency handling corrects them.
- `Proofs/public-parity.json`: public placements, native contracts, logic patches,
  behaviors, solver routes and tracker settings/topology exactly match the three
  corresponding internal integration cases. Fingerprints differ because the
  public requests record their actual options. No unrelated seeds rerun.
- `Proofs/Tracker/verification.json`: 267 item-progression steps plus objective
  visits pass the embedded native oracle; excluded checks stay unavailable.
- `Proofs/objectives-native-results.json`: source-written goals and counts,
  actual native WS decompression, quota-triggered escape, slot revalidation and
  complete restoration of the original runtime ROM pass on gaming-pc.
- `Proofs/profiles.log`: 66 profiles pass publication/reload, malformed-plan
  rejection and historical compatibility. Native, Game and Editor compile.
- `Proofs/render-results.json`: gaming-pc Vulkan run and asynchronous oracle pass.
  The original map and objective pause were visually inspected (PNG 290/200).
  The recorded executable predates the final editor-only dependency control;
  runtime/parsing code is unchanged by that final edit.

The transition and ending gates now have focused native evidence:
`Proofs/Transitions` contains three public-seed room changes initiated by the
original door-block collision, followed through game states 9/11/8 into the
source-written destination. Cases cover the Tourian exit, a cycling Flyway exit
and a Minimizer world. Departure room/inventory/quota are controlled fixtures;
no destination is injected during the actual door transition.

For Disabled Tourian, the original ship load station positions Samus; Down then
runs the unmodified ship AI, takeoff and ending state machine to real credits
(states 8/38/39, 3,210 frames). This tests the ending after a controlled quota and
return to ship, not a timed traversal of the entire generated escape route.
The initial test fixture incorrectly warped from a save station into Landing
Site, leaving Samus waiting for a nonexistent save-platform PLM. It was corrected
to use the real ship load station; no production workaround was introduced.

`Proofs/Public/seed-03.json` adds public seed 15093503: Escape + Fast Tourian +
Scavenger (four targets) + light area randomization. Full source progression,
100 embedded tracker steps and native contract/clock/map/WS composition pass.
Killing G4 alone correctly leaves the Scavenger quota unmet. These combined-mode
proofs cover generator, native routines and tracker; the earlier Vulkan proof
covers Disabled Tourian and is not relabeled as this seed.

Continue the full goal's Mirror, race and remaining noncosmetic patches. No
local game was launched; no runtime/release/user-save replacement was made.

Useful source seams: `GraphUtils.escapeAnimalsTransitions`,
`RomPatcher.applyEscapeAttributes` / `writeDoorConnections`,
`GraphBuilder.escapeTimer`, `objectives.asm:trigger_escape`,
`rando_escape_common.asm` and flavor `rando_escape.asm`.
