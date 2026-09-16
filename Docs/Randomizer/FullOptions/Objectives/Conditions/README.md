# Native objective conditions and Tourian gate

This is the earlier evaluator checkpoint. [Binding](../Binding/README.md) now connects the contract to standard seed generation and A/B/C profiles and corrects the public frame-hook cadence. The scope and proof below describe this earlier candidate.

This checkpoint implements the native evaluator; full objective integration remains unfinished. The production generation guards are unchanged. `sm_objectives.py` captures the proposed effective descriptor but is not yet called by `sm_integration.py`. Unreal profiles, live solver/tracker goals and the original objectives pause screen still need the same descriptor before the option can be enabled.

## Committed native behavior

`Native/sm_objectives.{h,c,inc}` and `Scripts/build-objectives.py` define a versioned 272-byte descriptor, with a catalog hash, ordered goal IDs, required count, hidden/SFX flags, actual-item and regional HUD-count membership, upgrade masks, enemy totals and map totals. Four pending configurations support A/B/C plus the generation staging slot. Activation validates the pending layout; only a successful seed/world transaction changes the applied evaluator. Vanilla and historical plans without a descriptor keep their original completion rules. The contract currently rejects filtered worlds and ordered Scavenger instead of misinterpreting them.

All 58 currently supported source conditions are evaluated in native C: boss/miniboss groups, item percentages, exact source upgrade masks, region clearing/exploration, total map percentages, robots, animals, special interactions and unique enemy families. The remaining catalog entry is ordered Scavenger. Conditions are latched one per gameplay frame using the existing ZOE1 completion events. A full cycle updates required/all-complete status and batches the original channel-2 particle sound. Later optional goals complete silently once the required quota has been met, matching VARIA. Pause and transitions do not advance checks. Hidden goals reveal in the Golden Statues Room for the supported ordinary-Tourian mode.

The generated bank-87 overlay conditionally sets the four original statue events at operands `$8402/$846A/$84D2/$853A`, while retaining normal instruction advancement. The ordinary Tourian-open event is set by the required objective quota. Existing statue HDMA, PLMs, collision changes and room loading then open the passage; no replacement geometry or emulated patch instructions are used.

## Evidence and limits

`conditions-results.json` covers all 58 supported conditions, threshold boundaries, read-only progress queries, all 18 completion slots, fractional item thresholds, exact upgrade equality, frame scheduling/SFX, hidden reveal, invalid descriptors, pending-world rollback, native A/B/C SRAM and six out-of-order reloads. Item and map membership are checked independently against the original location bits and source map ownership.

`tourian-results.json` uses the actual native room loader and 2,400 gameplay frames per scenario. With all four bosses defeated but the selected red-fish goal incomplete, all four floor blocks remain solid and the statues stay in place. With the red-fish goal complete and no bosses defeated, the original floor blocks become air and both vertical scroll regions open. Both native captures were inspected. This is a controlled objective fixture, not a full seed playthrough.

The same final library passes the existing objective-event suite (145 unique deaths/repeats, exact AI interactions and SRAM), complete map-exploration checks, original map-icon pixels, tracker and mixed-bank regression. An isolated Unreal Vulkan run loads a generated area connection and exercises original pause/navigation/resume; its capture was inspected. That render verifies compatibility with existing UI, not the still-missing objectives screen. The 40-transition suite from the prior event checkpoint was not repeated here.

`capture-results.json` covers effective source descriptors from three generated pools: Full, FullWithHUD plus area randomization and hidden randomized goals, and Chozo. Each generator uses a fresh process, matching the production bridge's fresh Python subinterpreter. This matters because VARIA stores active goals in class globals; reusing the test interpreter leaked the preceding seed's goals, so that fixture was corrected and requested-vs-effective goal identity assertions were added. The source writer can expand “kill all G4” into its four individual goals. Hidden goals only apply to randomized objective selection; fixed lists force them off. Native activation and objective-aware solver validation are deliberately not claimed by these capture tests.

The candidate hash and proof hashes are in `build-hashes.json`. Only `/tmp/sm-native-generation-20260914` on gaming-pc was used for runtime validation. No local game was launched or local packaged runtime replaced. `preservation.json` rechecks the frozen Save Refill binaries/ROM and all three pristine source revisions.

## Next integration steps

1. Bind `sm_objectives.capture` into generation and serialize the contract in the fingerprinted manifest. Capture the post-writer required count and effective expanded/selected goals, not the requested list. Preserve relic-hunt completion and historical manifests. Handle Bomb Torizo sleep and objective SFX-priority dependencies when enabling these goals.
2. Parse/validate the same contract in `FSMSeedPlan`; configure it in `FSMSystemMenu` after area routing and before world activation, including generation staging, A/B/C load/copy/clear, rollback and old-save defaults. Add the required-native capability only when this path is ready.
3. Replace the vanilla-goal assumptions in validation and live tracking, including effective item counts, goals, hidden status and required count. Verify several enabled seeds' complete progression and goal conditions end to end.
4. Reproduce the original VARIA objective pause assets, progress notifications and objective/boss map icons. Then complete ordered Scavenger, modified layouts/Minimizer, Tourian/escape/race and remaining non-cosmetic patches under the full goal.
