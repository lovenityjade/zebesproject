# Native Scavenger — 2026-09-15

Scavenger is connected to the public request resolver, source generator, native per-slot plans, original pickup/Ridley routines, native HUD, objective evaluator, saves, solver and embedded tracker. Full original/area layouts and ordinary Tourian remain the supported world boundary. This is one mode of the full VARIA integration; it does not complete the overall goal.

## Contract and compatibility

`native_scavenger.json` fixes the 17 original location IDs, source HUD indices/labels and exact source-written order words. Generation consumes `writeSplitLocs`, including its Hunt Over marker and padding. `nativeContext.scavenger`, the objective dependency and tracker settings must agree. Effective HUD, progression speed and hunt length are recorded separately from requested settings.

Native `SmScavengerPlan` is a separate 44-byte contract with A/B/C and scratch configurations. Objective schema/version 2 enables the existing finish-hunt identity 16 only with its validated hunt dependency. Objective catalog identity, 272-byte plan, 472-byte companion snapshot and historical tracker snapshot layouts remain unchanged. Older plans retain version 1. The saved index uses original VARIA word D8F4 inside the original checksummed SRAM block; no save-format expansion.

The native PLM denies future mandatory checks without granting items or changing RAM. Unrelated pickups remain available. Ridley waits until eligible, with his original door data allowing departure, and his native death routine advances the hunt. Pause X/Y browses remaining original location labels without changing the saved index. Finish-hunt progress and quota use actual native events, including the normal Tourian objective gate.

The solver traverses the serialized hunt order and rejects out-of-order progression. The live map/minimap tracker marks future mandatory checks unavailable (existing red state), using collection bits and Ridley's native boss bit. With the native companion snapshot it also checks the saved index; its ordinary stale-result rejection includes that progress. It never substitutes item ownership for check identity.

## Focused evidence

- `test-scavenger-native.py`: 2,601 ordered pickup/gate cases; actual Ridley appearance/death routine; seven source door-data bytes and complete ROM restoration; original checksummed A/B/C saves, invalid configurations and vanilla isolation.
- `test-scavenger-collision.py`: actual Morph-room denied and allowed collisions, native item grant, original pause X/Y and resume. A denied pickup is retriggered after room re-entry, as in the source. Start exit respects the original eight-frame debounce.
- `test-scavenger-public.py`: production seeds 15093100/101/102, with 4/8/17 required locations, original/random items, area/boss randomization. All 100 checks and completion verified by the source solver, with exact ordered progression.
- `test-scavenger-tracker.py`: tracker at mandatory progression boundaries and source objective visits; future checks unavailable and completed checks retained.
- `test-scavenger-embedded.py`: identical generation fingerprints/context/tracker through the native API used by Unreal.
- `test-scavenger-objectives.py`: actual generated placement/order contracts, native PLM gates, finish-hunt objective counter/latch and save/reload at every mandatory pickup, with zero emulated CPU opcodes. World traversal is tested separately in Unreal.

Runtime evidence is confined to `/tmp/sm-native-generation-20260914` on gaming-pc. No local game is launched and no local packaged runtime or user save is replaced. `Proofs/verification.json` ties final seed fingerprints to generation, native pickup/save routines, tracker queries and three Vulkan runs. All 57 profile fixtures pass, including the three new Scavenger plans, copy/reload/clear and malformed order rejection. The map and objective screenshots were inspected: original game assets, readable finish-hunt counter and source X/Y prompt. Native, Unreal Game and Unreal Editor builds pass. `Proofs/build-hashes.json` records the binaries and preservation audit.

## Next work in the unchanged overall goal

Modified/mirror worlds and Minimizer; alternate Tourian and escape; race settings; remaining relevant gameplay patches; their effective solver/tracker memberships and end-to-end proofs. Save-station travel remains paused and Speed Booster effects remain queued. The Save Refill release and pinned upstreams remain immutable.
