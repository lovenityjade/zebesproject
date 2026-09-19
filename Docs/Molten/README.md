# Molten liquids — original art with fluid motion

Lava and acid/magma retain their ROM sprites, palette, native animation and
actual liquid level. The existing Engine Weather switch controls the additions:

- Gentle two-dimensional distortion and broad, slow variations within the liquid.
- Sparse incandescent droplets that rise from exposed surfaces, follow an arc,
  cool and fall back. Emitters stay anchored to the room as the camera moves.
- Native HUD, Samus, enemies and foreground structures are excluded, including
  objects seen through the SNES liquid's subscreen blending.

The native bridge exposes the real lava/acid height in visual field 54. BG3
metadata records the underlying foreground/sprite when the liquid is blended
on top. This changes presentation metadata only. The material uses the alpha
channel of EnvironmentMask for exposed liquid, with a two-pixel sampling guard.
No gameplay RNG, damage, collision geometry or source sprite assets are changed.

## Verification

`Scripts/test-molten.py <isolated-root>` runs copied-save fixtures on gaming-pc:
Rising Tide, Lava Dive, Acid Statue, Maridia West and Green Brinstar.
The first three compare the effect off/on at identical frozen native frames;
a later capture advances the presentation clock to check animation. The last
two verify that enabling this feature does not alter water or dry rooms.
The native frame is intentionally frozen for pixel comparisons; this is not
a full manually played traversal of every lava room.

The submerged Lava Dive fixture specifically checks the objects behind the
liquid. Its first draft exposed that main-screen provenance alone was not
sufficient; the final mask includes native subscreen provenance as well.
Original scene bytes are compared before/after that metadata-only correction.
Test boot flags are restricted to non-Shipping builds and the isolated marker.
