# Candidate E / native pickup import

The user explicitly selected E at 16x16 on 2026-09-15. The reference is preserved in `../Reference/user-candidates.png`.

`fragment-e-16.png` is a technical import of E: crop 238x307 at (28,784), trim, nearest-neighbour fit within 16x16, binary alpha, no dithering, maximum 15 colors, centered transparent canvas. No generative redraw or smoothing is used. `fragment-e-preview.png` enlarges the same grid by 20 with nearest-neighbour sampling. Earlier A candidates and generated tablet drafts are not used by the game.

Runtime uses the exact 256 BGRA words in `Native/sm_relic_pixels.inc`. The native pickup stays 16x16 and respects the native reveal/collection script; glow and particles are separate Unreal effects. In-game visual evidence is recorded in `Docs/Randomizer/FullOptions/Proofs`.
