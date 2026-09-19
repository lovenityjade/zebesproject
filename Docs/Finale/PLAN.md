# Mother Brain final battle — active implementation

Applies to Vanilla, Randomizer and Boss Rush (confirmed by the user).
Preserve ROM-derived sprites, native encounter progression and save compatibility.
Do not modify live player saves. Build/test on gaming-pc only.

1. Expose actual draining, healing, final charge, sacrifice, transfer, Hyper Beam
   combat and boss death phases. Keep damage, collisions and Hyper Beam handoff.
2. Organic baby pulsation/suction; visible energy pulled from Mother Brain,
   restrained saliva droplets. Use original sprites, no generated anatomy.
3. Powerful eye beam against the baby's last charge. Frozen grayscale room,
   colored Samus/baby, music silence. Replace falling death explosions with
   luminous disintegration droplets descending onto Samus and empowerment.
4. Restore color/movement; play Face the Memory from 0:00 with a two-second fade-in, continuing through escape.
   This scene uses a separate cue; normal Boss Rush music stays unchanged.
   Rainbow Hyper Beam, strong muzzle glow and restrained visual recoil;
   bounded flesh-colored fragments/crimson droplets on actual Hyper hits.
5. Enhanced Mother Brain disintegration and room light changes; retain the theme
   through escape, suppressing the native Escape cue. Honor reduced-flash preference and rendering tiers.
6. Separate temporary save before the Mother Brain sector entrance; enter through
   a real door, preserve progression state and never overwrite the player's run.
7. Remote native progression/audio checks and rendered Unreal scene captures.
   Inspect results and correct defects before declaring ready.

Implemented and installed locally after remote validation. See IMPLEMENTATION.md
and the adjacent native/audio/render/save/Boss Rush verification reports.
The local game remains closed; Scripts/play-finale.sh selects the isolated save.
