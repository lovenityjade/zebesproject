# Settings and native start flow

Current front-end release: **ALPHA-0.24**. See its
[validation record](ALPHA-0.24-VALIDATION.md) for the updated native mode menu.

The system menu configures the game; the original title/file-select/options
screens start it. The system menu stays closed at boot. Escape, F4 or the right
stick opens it; Start still opens the original pause/map/equipment screens.

## Starting a game

1. Choose an empty SAMUS A/B/C slot in the original file-select screen.
2. Use Left/Right on the mode row: **Vanilla Mode → Story Mode → Boss Rush
   Mode → Randomizer Mode**. Story and Boss Rush are disabled previews.
3. Vanilla can start immediately. Randomized offers **Randomizer Options**, then
   **Generate Game**. Start remains locked until successful generation and save.
4. Generated slots retain their seed and rules. Generate is disabled afterward.

As of **ALPHA-0.24**, Vanilla only displays its description and Start Game.
Story explains the planned cinematics, voice acting and expanded story. Boss
Rush explains the minimum-item, single-attempt time trial and previews Easy,
Medium, Hard and Hardcore. Neither preview can start or overwrite a slot's mode.
Randomizer alone displays Randomizer Options and Generate Game. The former opens
the existing system UI, bound to the selected slot. Start Game stays at the same
bottom position above the native button legend. Controller Setting Mode has been
removed from this screen; bindings remain available in Settings → Controls.

Mode emblems use the original Samus helmet for Vanilla, the original menu star
for Randomizer, the approved tablet for Story and the tracker Ridley for Boss
Rush. Real A/B/C saves currently support Vanilla and Randomizer; the two future
emblems appear in previews, without introducing unsupported save types. Existing
nonempty or generated slots retain their mode and seed.

Preparing randomizer settings no longer requires creating a bank in the system
UI. Defaults are persisted and copied when a new slot switches to Randomized.
Opening the editor for an editable randomized slot, either through native
Randomizer Options or the system menu, binds the draft to that slot. Changes
are saved to its request. A/B/C remain independent; generated seeds are unchanged.

The Save library creates additional A/B/C banks when needed and retains
Vanilla speedrun categories, existing-bank loading and completed-game NG+.
The additional-bank action is not a prerequisite for ordinary play.

## Navigation

- Settings: Display; Atmosphere & effects; Audio; Controls; Gameplay & comfort;
  Map & trackers; Interface.
- Randomizer: Seed & sharing; Logic & difficulty; Progression; Items & ammo;
  World & escape; Goals & victory; Gameplay patches.
- Advanced randomizer pages: Techniques; Combat & heat. Every option is retained;
  the distinction is navigation only and never changes logic or defaults.
- Save library, Achievements, Session and Debug tools remain separate destinations.

A persistent side navigation replaces the two stacked tab bars. Narrow windows
use section/category dropdowns. Each page retains its own scroll position.
Search spans settings, randomizer options, saves and debug tools with separate
widget IDs. Keyboard and controller binding buttons identify their device.
Chozo Tablet Hunt labels consistently refer to tablets and explain quota escape.
Text scale compensates for the game's Slate DPI curve, so a smaller window
reflows the settings instead of shrinking every label. This affects only the
system menu; game pixels and the native HUD keep their own scaling rules.

## Validation

Run only on gaming-pc in a marked isolated fixture:

```
python3 Scripts/test-menu-flow.py /path/to/isolated-root
python3 Scripts/test-native-slots.py /path/to/isolated-root
python3 Scripts/test-mode-presentation.py /path/to/isolated-root
```

`-SMMenuFlowTest` checks the real native menu with the Unreal profile callback:
closed system menu at boot, native file selection, Vanilla start eligibility,
hidden Vanilla randomizer rows, blocked Story/Boss Rush previews, all four Boss
Rush difficulty choices, copying configured defaults into Randomized A,
pending-start gating, native-to-system editor handoff, draft persistence and
B/C isolation, reopening from Escape, and switching back to Vanilla to start.
It creates a fixture bank and requires the isolated-root marker.

The render runner captures common, advanced, search and narrow-window pages.
Build success alone does not establish layout quality or controller navigation.

### September 16 results

- SMUnreal Game and Editor Development builds passed on gaming-pc.
- Native menu plus Unreal profile flow passed in Game and Editor hosts. Editor
  used NullRHI; Game used Vulkan. The fixture exercises native input, not just
  profile metadata calls.
- Existing A/B/C test passed: mixed modes, independent seeds, generation gating,
  initial save, copy/clear and commit failure protection.
- Fifteen Game-host captures: overview; display; effects; audio; controls;
  comfort; trackers; interface; randomizer seed, logic, goals, techniques and
  combat; search; small-window layout at 640x480. Normal captures use 1280x720.
  Representative common, advanced, search and small layouts were visually read.
- Source catalog and gameplay rules are unchanged. This is not a new proof of
  every seed setting combination or a full manual controller-navigation audit.
- The first Editor Vulkan attempt exceeded the fixture's 180-second limit while
  compiling a fresh global shader cache. Its remaining isolated shader workers
  were stopped; that attempt provides no rendered evidence. Editor flow was
  then checked with NullRHI, separately from the Game rendered captures.

Local evidence: `.tmp/menu-start-flow/`. Remote evidence:
`/tmp/sm-native-generation-20260914/menu-flow-results/`.
Set `SM_MENU_EDITOR` to the engine's UnrealEditor executable to run the flow
fixture against the isolated `EditorMenu/SMUnreal.uproject`; the runner uses
NullRHI for that case.
