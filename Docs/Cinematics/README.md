# Cinematic atmosphere and gunship propulsion

The original Super Metroid sprites, palettes, animation and Mode 7 transforms remain the source image. The Unreal material adds soft light at output resolution over the existing Gaussian **Lighten 110%** treatment. No replacement or generated artwork is used.

## Presentation

- Front-view gunship: two animated exhaust plumes, blue-white cores, warm halos and small streaming particles.
- Rear-view gunship: one stronger central exhaust, following the native rotation and distance.
- Crateria gunship: two larger exhaust sources during descent, landing stabilization, charging and departure. An idle parked ship does not emit thrust.
- Ceres: soft red light at the original docking-ring lamps. Their current palette controls the light, including the native flashing/fades. Asteroid highlights are not station beacons.
- Zebes approach: a warm atmospheric rim around the original planet, following its actual sprite position; original stars receive soft animated glints.
- Ending: existing native escape scenes receive the same ship/star treatment and the early planet view receives an atmospheric rim. The native explosions, timing, subsequent credit roll and final Samus sequence continue unchanged.
- Story pages retain the established Gaussian treatment. Original story text and Ceres/Zebes/ending captions are isolated from the blur and lighting.

**Settings → Graphics → Atmosphere & lighting** controls this presentation together with the existing atmosphere pass. The existing blend/intensity choices remain available. Turning atmosphere off restores the native image. Neither rendering metadata nor shaders change collision, ship movement, saves, seeds or statistics.

## Source anchors

`Native/sm_cinematics.c` exports a 32 × 3 RGBA float light texture: geometry/type, color/strength, direction/length/phase. Native cinematic setup and actual sprite-spawn hooks identify the current scene; there is no elapsed-time guess for runtime scene selection.

Ship and Ceres artwork comes from native tiles **95:A82F**, tilemap **96:FE69**, palette **8C:E5E9**. Tilemap offsets `0x000`, `0x300`, `0x600` identify the front ship, rear ship and Ceres respectively. Nozzle/lamp coordinates are projected through the inverse of the actual PPU Mode 7 matrix, including flips and scroll clipping. Ground gunship anchors follow its real enemy coordinates and camera, using the larger gameplay sprite's apertures.

The planet uses the original approach backdrop (tiles **96:EC76**, map **97:8ADB**) and cinematic sprite `8B:CEA3`. Stars are small connected components in the original rendered scene, excluding protected text, planet interiors and the transformed ship/station footprint. No random replacement star field or synthesized planet is introduced. All added source strengths follow native screen brightness.

`Shaders/Cinematics.usf` is assembled into the existing presentation material by `Scripts/create_material.py`. Effects are evaluated at the display resolution; nearest-neighbor native pixels remain the base layer. The credits' own atmosphere is exclusive with cinematic lighting, including a suspended debug preview.

## Validation and fixtures

Runtime checks run only in the marked isolated directory `/tmp/sm-native-generation-20260914` on **gaming-pc**, without launching an interactive window or changing the installed game/user saves.

- `Scripts/test-cinematics-native.py`: intro ship, Ceres escape, Zebes, landing, story and controlled ending samples. Scene + protected GUI reproduce the native RGB bytes in cinematic samples; native CPU opcode count stays zero.
- `Scripts/test-story-presentation.py`: actual fresh-game story progression through the original pages to playable Ceres, constant wide canvas and protected text composition.
- `-SMCinemaTest -SMCinemaCase=N -SMCinemaSample=N`: actual Unreal/Vulkan captures of a frozen native frame with atmosphere off, Gaussian alone, full cinematic effects, then six native frames later.
- `Scripts/check-cinematics-captures.py`: compares the new effects against Gaussian alone and checks exact GUI preservation.

The controlled fixture API accepts only isolated `SMTests` save paths. It resets gameplay HDMA/camera before cinematic entry; Ceres escape sets the actual escaping-Ceres loading state so the following landing is exercised. Ending entry uses the real state-38 fade/cleanup. These fixtures are not a complete campaign playthrough.

Recorded results and reviewed captures are stored alongside this document.

### Recorded evidence

- [Builds, hashes and validation scope](build-evidence.json)
- [Native cinematic samples](native-verification.json) and [fresh story traversal](story-verification.json)
- [Vulkan pixel comparison](visual-verification.json) and [credits recovery regression](credits-regression.json)
- [Before/after comparison](comparison.png), [intro propulsion](intro-propulsion.png), [Ceres](ceres.png), [Zebes](zebes.png), [landing](landing-propulsion.png), [ending propulsion](ending-propulsion.png)

The eight Vulkan cases each compare four captures. Reviewed images cover front and rear ship views, Ceres, Zebes approach, ground descent, story text and the ending ship. Across sampled story/caption/HUD pixels, the shader preserves the native UI exactly. The difference report measures the additional cinematic pass independently of the already accepted Gaussian filter.
