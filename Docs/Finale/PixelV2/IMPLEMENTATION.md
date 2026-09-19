# Mother Brain finale — pixel revision

Requested changes: replace the smooth Hyper Beam and geometric death debris with textured pixel effects; retain the upgraded Hyper outside Mother Brain's room; make the baby's draining of Mother Brain visibly directional; start Face the Memory at 0:00 and keep it through escape; add unstable escape lighting and background fireballs at the Crateria surface.

## Presentation

- `SMFinalePixels.cpp` composites the ImageGen atlas at 256/400 × 240 before the existing game lighting and softening pass. Each effect shares the game's pixel grid. Alpha edges are cut cleanly and RGB is quantized to 5-bit channels. No new full-resolution beam ribbon overlays.
- Hyper art replaces only native projectile rendering after the atlas loads successfully. Collision, damage, firing cadence, weapon ownership and directions stay native. Missing artwork retains the original projectile. The effect follows Hyper ownership in every room.
- The drain progresses according to the native Mother Brain script stages. A desaturated petrification front climbs from the lower body towards the brain while upward veins/arcs weaken. The baby's original red nuclei pulse as energy arrives.
- Dissolution exposes reconstructed original room layers, with clustered pixel cracks, shaded fragments and embers rather than flat-color noise. The final corpse uses the native brain draw-hook palette, which switches to OBJ palette 7 at death. This excludes the lingering black BG2 body mask and unrelated projectiles. The PPU layer/OBJ fields are restored immediately after background reconstruction.
- Escape lighting uses overlapping slow local dimming circuits and warm emergency modulation. The emissive and light textures follow the same flickering circuits. It excludes the HUD and actors, respects FlashStrength, and avoids a full-screen strobe.
- Surface fireballs are cosmetic and clipped behind foreground terrain and actors. Their world-space impact point stays fixed after spawning. Native collision and route geometry are untouched. Particle counts scale down on Low.

## Audio

The finale starts a separate cue from sample zero with a two-second fade. Its music ownership survives room changes and pause, suppressing the Escape music command until the ship cinematic. Ordinary Boss Rush music still uses its own original cue; its result handoff and finale fade remain as before. Native SFX stay enabled.

## Validation scope

Builds and automated gameplay/render checks run on gaming-pc in the isolated release-alpha025 checkout. Audio checks cover exact start/cursor, sacrifice silence, pause, room transition, Escape command suppression, ship cinematic release and ordinary Rush restart. Render fixtures script encounter setup, survival, shooting and/or an outdoor escape state: they are visual/regression evidence, not human playthroughs or final user approval.

Asset provenance and exact generation prompt: `ASSET-PROVENANCE.md`.
