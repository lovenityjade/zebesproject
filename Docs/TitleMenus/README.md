# Original title and native menu atmosphere

Historical renderer notes: the original title presentation described below has
been superseded by the [approved layered title and startup notices](../STARTUP-TITLE.md).
The save selector and Option Mode rendering described here remain in use, with
the [ALPHA-0.24 mode-selector layout](../MENU-START-FLOW.md). Turning
Atmosphere off disables the new title effects; it does not restore the old artwork.

The title screen, native A/B/C save selector and the following **Option Mode** screen now share the established Gaussian atmosphere and source-anchored lighting. The original assets, font, layout, selection controls and save/randomizer logic are retained.

## Appearance

- **Title laboratory:** the two terminals, Metroid containment chamber and blue machinery emit soft light. Their actual rendered palette drives the color through the opening animation. The lights follow the native Mode 7 zoom and camera. A restrained, smoothly animated vapor layer lies across the laboratory floor. The original logo receives the Gaussian treatment; the small Nintendo copyright text remains exact.
- **Save menu and Option Mode:** the existing Zebes backdrop receives the same warm planetary rim and animated star glints used in the approach cinematic. The original background is rendered behind the menu foreground, rather than blurring the assembled text screen.
- **Submenus:** Data Copy, Data Clear, controller settings and their transitions use the same separation. Native borders, labels, missile cursors, helmet icons, randomized badges and disabled option colors remain in the protected foreground.
- **Randomizer states:** Vanilla, pending, generating, failed and generated states keep their original availability rules and readable colors. Lighting never makes a disabled option look enabled.

This uses **Settings → Graphics → Atmosphere & lighting**, together with the existing Lighten/Multiply and intensity settings. The reference captures use **Lighten 110%**. Atmosphere can be disabled for the original presentation. The title's native animations, palette effects, zooms, fades and demo timeout keep running normally.

## Original sources and rendering

The title laboratory comes from native Mode 7 tiles **94:E000**, map **96:FC04** and the title palette at **8C:E1E9**. The projected light anchors are the terminal displays `(70,152)` / `(185,147)`, containment tank `(128,142)` and blue equipment `(128,183)` / `(128,97)`. The floor vapor follows `(128,205)` through the same transform. Native copyright sprite definitions `8B:A113` and `8B:A125` are tagged for the protected GUI layer.

The save/options background is the actual **8E:DC00** Zebes-and-stars tilemap loaded by `FileSelectMenu_1_LoadFileSelectMenuBG2` and `GameOptionsMenu_1_LoadingOptionsScreen`. Its planet has the original `(123,99)` center and approximately `(49.5,52)` radii. Small original star components receive animated glints; text and menu icons do not become light sources.

`Native/sm_scene.c` detects the real mode-1 menu layout: BG1 at word `0x5000`, with BG2 scenery at `0x5800`. It saves the visible BG1/OAM foreground into the GUI texture and renders a PPU copy containing the original backdrop for the atmosphere pass. It does not modify native PPU state or VRAM. The original RGB image remains available. `Native/sm_bridge.c` makes states 2 and 4 eligible for atmosphere while keeping them outside gameplay weather/combat rendering.

`Native/sm_cinematics.c` supplies the existing light texture. `Shaders/Cinematics.usf` adds only a smooth floor-vapor source type to the existing effects. No image generation, replacement logos, procedural tiles, extra menu panels or external asset packs are involved.

## Validation

Runtime tests run only in the marked isolated directory on **gaming-pc**. User saves and the installed game are not modified; no interactive game window is opened.

`Scripts/test-title-menus-native.py` reuses the A/B/C fixture in a separate `title-menu-results` directory. It traverses the full natural opening, title, save menu, copy/clear pages, Vanilla/Story/Boss Rush options and randomizer generation states. At each capture, scene + protected GUI must reproduce the original RGB bytes. It exercises the actual native navigation and generation commit with existing validated fixture placements; it does not run a new seed-generation algorithm test.

The existing Unreal offscreen fixture accepts `-SMCinemaTest`, with:

| Case | Sample | Capture |
| --- | --- | --- |
| 4 | 1000 | Early METROID 3 card; Gaussian only |
| 4 | 1400 | Opening laboratory zoom |
| 4 | 1750 | Ready title screen |
| 5 | 90 | Save selector |
| 6 | 90 | Following Option Mode screen |
| 7 | 90 | Story preview (disabled) |
| 8 | 90 | Boss Rush Easy preview (disabled) |
| 9 | 90 | Pending Randomizer Mode |
| 10 | 90 | Boss Rush Hardcore preview (disabled) |

Each case captures atmosphere off, Gaussian alone, full effects and six native frames later. The pixel checker compares the new effects independently of Gaussian and verifies unchanged GUI pixels. Previous gunship and Zebes cinematic cases provide a shader regression check. Controlled fixture entry is separate from the full natural-opening traversal.

### Historical evidence before ALPHA-0.24

- [Build hashes and scope](build-evidence.json)
- [Native menu and randomizer-state checks](native-verification.json)
- [Vulkan GUI preservation and effect comparison](visual-verification.json)
- [Overview](overview.png), [title](title.png), [save menu](saves.png), [Option Mode](options.png), [controllers](controllers.png)

The Vulkan menu fixture has Vanilla A/C and an ungenerated Randomized B, using the normal native managed-slot renderer. The native navigation test separately exercises a mixed bank and all generation states. The layout stays native inside the wide canvas; this update changes presentation only.
