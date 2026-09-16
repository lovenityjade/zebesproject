# Display scaling

`Settings > Graphics > Image scaling` offers:

- **Fit to screen** (default): largest proportional image that fits the viewport, with fractional scaling when needed. No stretch or crop. Small margins remain when the display and game aspect ratios differ.
- **Integer scaling**: whole-number enlargement for uniform pixel blocks, centered with black margins. Windows smaller than the source image fall back to proportional downscaling instead of clipping the HUD.

The selection applies immediately to the game presentation, including native menus and cinematics, and is stored as `[Display] ImageScaling=0` (fit) or `1` (integer) in `Unreal/Saved/SM/Presentation.ini`. Widescreen and window mode remain separate settings. Existing rendering effects and texture filtering are unchanged.

At 1280x720, the 400x224 wide view uses 1280x716.8 in Fit mode versus 1200x672 in Integer mode. At 1920x1080, Fit uses 1920x1075.2 versus 1600x896 in Integer mode. The native-width view retains its own proportions.

F11 and the Window mode menu use the same saved borderless/windowed preference. `Scripts/play.sh` no longer overrides saved window mode or resolution; first-launch defaults live in `Unreal/Config/DefaultGameUserSettings.ini`. Explicit command-line display options still work.

Existing automatic pixel fixtures retain integer scaling and explicit 1280x720 windows. Isolated development runs can use `-SMImageScaling=0` or `1` with an existing automatic test to verify both render paths. The override is accepted only with `ISOLATED_TEST_DIRECTORY` present.
