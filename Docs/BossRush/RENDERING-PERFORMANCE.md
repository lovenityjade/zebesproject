# Rendering performance and quality controls

## Scope

The wireframe renderer is shared by Boss Rush arena transitions and its visual
lab. Gameplay logic, boss timing, damage and leaderboard rules are unchanged.
Rendering settings are global presentation preferences, not seed settings.

## Renderer

The old renderer used a rotated Canvas tile for every line. Each rotation
changed the Canvas transform and prevented ordinary batching. Dense rooms used
tens of thousands of lines, with extra outline/depth/glow passes.

The new renderer computes ribbon corners in screen coordinates and submits
triangle arrays with a common transform. Lines and textured glows are grouped
separately. A flush before Samus preserves arena/sprite layering; a final flush
submits Samus and the scan edge. There are at most four grouped wire/glow Canvas submissions
per transition frame (in addition to the captured scene/sprite textures); the engine may internally split their indexed buffers.
This count is not a claim of exactly four GPU draw calls.

## Settings > Display

- Rendering quality: Low, Medium, High, Ultra, Custom. Explicit presets apply
  Unreal scalability plus the project's weather/depth/relief choices. Existing
  installations start at Custom, preserving their authored visual preferences.
- Transition detail: Low preserves silhouettes but keeps one quarter of interior
  edges and omits glows/depth; Medium keeps half the interior edges, sparse glows
  and no depth struts; High keeps the original dense geometry and lighting;
  Ultra retains that geometry with more endpoint glows. Samus keeps her detail.
- Frame rate limit: 30, 60 (default), 120, 144, Unlimited. Simulation timing is
  independent of presentation. VSync remains separately selectable.
- Window resolution: 960x540, 1280x720, 1600x900, 1920x1080, 2560x1440. This is
  disabled in borderless mode, which follows the desktop resolution. Integer
  image scaling remains available. No misleading Canvas render-scale slider.

Preferences are stored under `[Rendering]` in the project settings; window,
VSync and Unreal scalability use GameUserSettings. Explicit changes to the
transition detail or atmospheric effects select Custom. Brightness, Gaussian
blend strength, accessibility flash strength and gameplay settings are not
modified by a quality preset. English and authored Canadian French are included.

## Verification

Builds and automated runs take place in the isolated gaming-pc checkout. The
user's active local executable is not replaced during profiling. Performance
results are recorded in `rendering-performance-verification.json`. The profiling
runner records actual quality tiers and refuses a requested tier mismatch.
`render-settings-verification.json` checks the four actual Unreal scalability
levels, all five applied frame caps and a configuration write/read cycle.
The graphics menu was rendered and visually inspected.

Reproduce the comparison using `Scripts/test-boss-rush-unreal.py` in the
isolated gaming-pc checkout; use `--quality 0..3 --fps 30|60|120|144` for
explicit renderer tiers. Archive each run's `unreal.log` before starting the
next. `Scripts/summarize-rush-performance.py <log-directory>` summarizes the
named logs. `Scripts/test-render-settings.py <isolated-root>` verifies settings
and captures the menu. Automated test frame caps are not overwritten at boot.
The local launch script no longer forces 60 FPS over saved user preferences.

## September 19 results

At 1200x672 on gaming-pc, using the same integrated traversal and a 30 FPS
cap, forward sweep + morph averaged 4.7–9.2 FPS for encounters 1–9 before
batching, and 29.8–30.3 after batching at High. The initial black entry reached
25.7 FPS after batching; it includes startup/capture overhead.

With a 60 FPS cap, verified Ultra runs averaged 56.3–57.7 FPS on encounters
1–9 (initial entry 49.3). Low averaged 55.0–58.7 (initial entry 49.6). These
runs include screenshot readbacks and injected gameplay; they are not a promise
of locked 60 FPS on other hardware. GPU utilization on the local machine has
not been remeasured with this version.

Dense-frame CPU submission cost at High: median 2.601 ms, p95 4.519 ms.
Low: median 1.322 ms, p95 2.567 ms. Maximum submitted triangle counts were
58,732 for High and 17,552 for Low. These are CPU submission costs, not full
frame/GPU times. High and Low Crocomire morph screenshots were inspected.
All three final quality runs completed all ten arena transitions, death and
Continue/Pause/Retry/End checks. The test harness now validates the actual tier
reported by the renderer, rather than relying on the command-line request.

The verified native and Unreal libraries were installed locally after confirming
the user's game had closed. Previous libraries were backed up. See
`rendering-installation.json` for hashes and backup path. No local game was
launched, no local compilation was run, and no commit/release was made.
