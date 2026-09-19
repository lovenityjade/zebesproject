# Mother Brain — death beam presentations

Three in-engine proposals for the native rainbow-beam sequence immediately
before the baby Metroid intervenes. No external VFX packs or generated images
are required: Unreal draws procedural soft light, perspective particles,
energy filaments, aperture rings and impact sparks over the original scene.

| Demo | Direction |
| --- | --- |
| [01 — Spectral Lance](Videos/01-Spectral-Lance.mp4) | Broad blue energy field, spectral filaments, white core and pink impact light. Closest to the original rainbow palette. |
| [02 — Stormbreaker](Videos/02-Stormbreaker.mp4) | Narrower violet/cyan beam, branching electrical filaments and an unstable impact. |
| [03 — Cataclysm](Videos/03-Cataclysm.mp4) | Heavy red/gold discharge, traveling compression rings and a white-hot core. |

`SMDeathBeam.cpp` contains the presentation. **Cataclysm (style 3) is now the
selected gameplay default.** The original proposals above remain as review
history; styles 1 and 2 are available only through the isolated recording fixture.
No new player-facing graphics option has been added.

## Selected version

[Cataclysm — growing orb and organic body colors](Videos/04-Cataclysm-Integrated.mp4)

The charge is a continuously growing red/gold light orb with a white center.
There are no orbiting particles, spinning filaments, aperture rings or traveling
rings in this version. The discharge retains its red/gold envelope, straight white-hot core and
outward impact sparks. Mother Brain’s existing rainbow body colors receive
a subtle saturation increase, varying slowly across the flesh. This is a
surface-color adjustment restricted to her body layer during the native
rainbow attack: no added emission, halo or room illumination. Original hue,
shading, outlines and brightness are retained (apart from channel rounding
and gamut clipping). Native timing
still determines when the charge becomes a beam; no new firing delay is added.

## Native integration

`sm_visual_state(51)` reads the actual boss routine (charge, firing or taper).
Fields 52–53 expose the eye's native beam origin. Samus remains the target even
while the original script pushes her against the wall. There are no gameplay
writes in the effect renderer. Native music, sound, damage, ammo drain, boss
animation and the baby Metroid sequence continue unchanged.

With enhanced combat effects enabled, only the presentation PPU copy loses the
old beam's color-math/window effect. Recovery states clear only the residual
HDMA fixed color, preserving the subscreen composition of the boss and room
while avoiding a flat red background after the channel ends. This suppression
also remains active during automatic reserve-tank recovery (native state 27).
The original PPU output is retained.
Disabling the existing atmosphere/effects option restores the original beam.
Broad light and the brief initial flash follow the existing `FlashStrength`
setting; the effect does not cover the HUD.

## Reproducible videos

Run `Scripts/test-motherbrain-beam.py <isolated-root> 0` and `... 1` on gaming-pc
to compare 1,500 native frames with enhanced rendering disabled/enabled.
The fixture loads Mother Brain's room and runs her real ascent and beam
routines, skipping the preceding fight. It uses a copied save only.

`Scripts/record-motherbrain-beams.py <isolated-root> [1 2 3]` records all three
Unreal variants, one at a time. Both fixtures require the isolated-test marker.
Recording flags are gated to non-Shipping builds and that isolated directory.

Each movie has 750 frames at `44100/1472` fps (approximately 29.96 fps), about
25 seconds, 1200 × 672 pixels, H.264/AAC. Two native frames advance per rendered
frame; native 44.1 kHz stereo audio is recorded alongside the pictures. This
keeps the original pace even when writing screenshots takes longer in real
time. No interpolation, fabricated combat footage or post-production VFX.

The fixture validates this cutscene and its presentation, not a complete
manually played Mother Brain encounter. Player saves and ROM are preserved.
