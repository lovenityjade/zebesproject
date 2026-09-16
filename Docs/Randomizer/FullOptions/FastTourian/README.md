# Fast Tourian — native integration, 2026-09-15

Fast Tourian is the next completed dependency toward Minimizer. It does not enable Minimizer, Mirror or Disabled Tourian. Public `tourian: Fast` requests now retain source VARIA logic patches, effective objectives, native behavior, per-slot plans and matching tracker settings. The ordinary escape and original item locations remain intact.

## Source-faithful behavior

- The Statues Hallway leads directly to Tourian Eye Door Room, with the reverse connection restored. Only six door/list bytes and the 749 authored Open Zebetites room-data bytes are applied; no 65816 code is executed.
- Original source door setup reveals hidden objectives and opens the eye after the objective quota. The eye's left-door hit instruction follows the exact source replacement at 84:8AA3: advance three operand bytes and return, without counting hits or opening the door. Shooting cannot bypass the quota. Leaving the eye room marks the glass/Zebetite events and refills only when area randomization is active.
- Native Mother Brain AI takes the source fast branch after phase 2, without the rainbow drain/baby cutscene. The original death sequence grants Hyper Beam, clears charge flares, animates health refill, restores the suit palette and refills health/reserves before the ordinary escape.
- The original objective pause screen displays `Tourian: Fast`. Non-area layouts remove exactly the now-unreachable G4 map tile from exploration counting; area layouts retain their source totals.

Objective schema/plan version 3 carries the Fast Tourian flag and can include the independent Scavenger contract. Historical versions 1/2 remain valid. All objective, tracker and SRAM struct sizes are unchanged. Turning the mode off restores original ROM data. Fast Tourian requires native objectives; incompatible or disagreeing contracts are rejected.

## Focused validation

`test-fast-tourian-public.py` uses production seeds 15093200 (no extra objectives), 15093201 (area randomization and map exploration objective), and 15093202 (Scavenger). Each serialized placement set has all 100 checks and completion solved; `test-fast-tourian-embedded.py` reproduces the manifests through the actual native generation API. `test-fast-tourian-tracker.py` checks all 300 item progression steps and objective visits.

`test-fast-tourian-native.py` calls actual original door/PLM/Mother Brain routines in both layouts, validates map totals and complete ROM restoration, and confirms zero emulated CPU opcodes. `test-fast-tourian-traversal.py` physically traverses the authored corridor into the shortened Tourian, then loads the Mother Brain room with controlled equipment. This is not a complete manual boss fight or playthrough. Test warps use real incoming door descriptors; the normal game does not use their whitelist.

All runtime work is isolated on gaming-pc. All 60 profile fixtures pass, including new Fast Tourian and combined Scavenger plans. Three Vulkan runs validate original pause navigation and the real asynchronous tracker. The `Tourian: Fast` / Scavenger 0/4 screenshot and native Mother Brain room were inspected. Native, Game and Editor builds pass. `Proofs/verification.json` records hashes and scope: the final eye-hit guard correction was verified with the native routine suite; the unchanged generation/tracker/UI suites were not repeated after that isolated correction. Their original binary hashes remain explicit. No local launch, local packaged-runtime update or user save modification. The frozen Save Refill release and pinned upstreams remain unchanged.

## Next in the full goal

Minimizer still needs effective region/location membership, self-loop routing for omitted areas, native boss exit behavior, filtered enemy/map totals and tracker visibility. Mirror requires its authored world data and native differences. Disabled Tourian needs the objective-triggered escape plus full escape routing/timing; race and remaining gameplay patches follow. None of these is claimed by this Fast Tourian increment.
