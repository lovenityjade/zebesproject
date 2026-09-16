# Minimizer — native integration

The runtime now supports VARIA's mixed 40-endpoint area/boss domain. This is a
separate catalog and 152-byte plan; the published 32-area and 8-boss catalogs
and historical save formats are unchanged. Public Minimizer requests now use the ordinary generation service. Enabling it
selects mixed area/boss routing and disables suit restrictions; targets below
100 force Full placement unless FullWithHUD was chosen. Random targets use
VARIA's 30–60 range. Requested settings remain separate from effective settings.

Implemented:

- Source-written mixed descriptors, including self-loops, orientation changes,
  spawn corrections, CRE reload, music and exit background restoration.
- Original area and boss blinking PLMs; native early boss-death hooks from
  `minimizer_bosses.asm`, Phantoon door preservation and Varia exit restoration.
- Source-written retained locations and graph regions. Excluded locations keep
  their original address/bit identity but disappear from native check markers
  and region counts. The oracle reports them explicitly as excluded.
- Mixed map portal destination sprites and relocated exploration marks.
- Filtered exploration: retained regions plus the exact 14 boss/post-boss tiles;
  source enemy totals and objective schema 4 with unchanged 272-byte ABI.
- Fresh solver completion uses retained checks and reachable miniboss regions,
  while keeping the selected difficulty ceiling. It still requires the actual
  Mother Brain and ship steps for these Fast Tourian runs.
- Typed Unreal parsing and A/B/C plans, generation configuration, copy/clear,
  reload and exact topology/objective/tracker contract comparisons.

Evidence on gaming-pc only:

- Three internal integration requests, seeds 15093300–15093302, regular skill,
  hard maximum difficulty, Fast Tourian; 30/60/100 targets yield 42/64/100 checks.
  The harness overrides the public option guard, explicitly **not** proof of
  production configurator readiness. Source routes reach all retained checks,
  Mother Brain and the ship. The initial medium-difficulty small seed exceeded
  the combat ceiling; that rejection was retained, not bypassed in production.
- All 206 retained check progression steps and source objective visits pass
  the embedded live oracle; excluded entries cannot appear reachable.
- 120 native mixed arrivals, filtered map/count membership, four bank switches,
  pending-plan rejection, actual Kraid callback, Varia exit routine, complete
  ROM restoration and zero interpreted CPU opcodes pass. This is controlled
  routine evidence, not a full physical playthrough or proof of every boss hook.
- 63 Unreal profile fixtures pass, including these three new plans, persistence,
  copying/clearing and historical plans. Native, Game and Editor builds pass.
- One actual Unreal Vulkan run passes asynchronous oracle and original pause
  navigation. Map and objective captures were inspected: original assets,
  filtered markers, four G4 goals and the Fast Tourian label. This does not
  establish the still-pending filtered enemy/objective icon cases.

Production continuation (seeds 15093400–15093402):

- Public requests through the native generation API reproduce exact fingerprints,
  contracts and tracker data. A 30-target Chozo request resolves to Full; a
  60-target exploration goal and a 100-target Scavenger hunt retain their goals.
  Effective worlds contain 30, 72 and 100 checks and reach their completion steps.
- All 202 retained tracker progression checks and source objective steps pass.
- 120 native descriptor/arrival checks, all four original boss-death callbacks,
  the boss save PLM, filtered counters and transactional bank switching pass.
- 54 source-writer comparisons prove filtered objective sprite pixels, including
  absent markers for enemy groups in excluded regions.
- A generated area-to-boss collision traverses the original transition pipeline;
  its source load/inventory are controlled, not a claim of manual playthrough.

See `Proofs/Public` for final production evidence; earlier internal artifacts
retain their own fingerprints and binary hashes. The public runs also pass 63 Unreal profile fixtures (including copy/clear and
reload) and an inspected Vulkan exploration-objective run with the asynchronous
oracle. Twenty-three settings checks cover reproducible 30–60 random targets,
dependencies and preservation of non-Full splits at 100 locations.

The full goal also still includes Mirror, Disabled Tourian/escape/race and
remaining noncosmetic patches. Frozen Save Refill and pristine upstream hashes
were rechecked unchanged. No local game launch or packaged-runtime replacement.
