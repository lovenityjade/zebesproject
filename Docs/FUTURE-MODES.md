# Future modes and multiworld

Recorded September 16, 2026 at the user's request.

Status updated for ALPHA-0.26: Boss Rush is playable as an alpha. Story and
Multiworld remain future additions. This document separates future scope from
the current implementation.

## Boss Rush

- A timed boss-rush challenge.
- Each boss is fought with the minimum equipment needed to defeat it.
- A dedicated leaderboard.

Research collected September 18: [minimum equipment by boss and miniboss](BossRush/MINIMUM-EQUIPMENT-RESEARCH.md).
This separates offensive requirements, arena hazards and survival thresholds.
Development started September 18: [five difficulty rules and implementation status](BossRush/IMPLEMENTATION.md).
Easy/Medium/Hard/Very Hard/Hardcore are playable. Encounter loadouts, menus,
transitions, death/results and focused combat checks are documented in the
implementation report; complete human playthroughs still need more coverage.

To define: what counts as minimum equipment, initial energy/ammo, boss order,
timing boundaries, permitted techniques/assists and ranking validation.

## Multiworld compatibility

- Support SekaiLink: Rebooted.
- Support Archipelago.
- Integrate multiworld into the port's gameplay and saves.

To define after studying both systems: supported game modes, connection and
session setup, item/check mappings, delivery and reconnection behavior. A common
integration layer with separate adapters is a design proposal, not an existing
implementation or a confirmed protocol contract.

## Story Mode

- A Vanilla-based experience with additional scripted cinematics.
- Four different endings.
- The largest and most meaningful of these additions in the user's stated vision.
- Preserve the project's established fidelity to the source material.
- Dialogue presentation approved September 16: compact full-width bottom box,
  portrait on the left, two lines of text on the right, no speaker name, native
  frame/font and blinking yellow downward arrow. See the
  [approved dialogue lab reference](../Labs/Dialogue/README.md).
- The [dialogue foundation](StoryMode/DIALOGUE.md) is implemented as a dormant
  native/Unreal scripting API. No story scene or trigger is included, and Story
  Mode remains unavailable in the menu.

To define with the user: the story, cinematic events, the four endings and their
unlock conditions. Whether endings depend on choices, actions, discoveries or a
combination is still open. No proposed plot or ending is approved by this note.

Boss Rush development started September 18. No release date or implementation
order for the other modes has been set.

Boss Rush implementation update (2026-09-18): playable arena adapter and native
menu activation implemented. Ten arena loads, native boss death scripts, VR
transitions, death presentation and practice isolation passed automated checks
on gaming-pc. Full player runs and minimum-kit balancing remain to be validated.
See `BossRush/IMPLEMENTATION.md`. Practice attempts are unranked; leaderboard
submission is not connected yet.

## Additional optional QoL — September 19, 2026

Requested future options for Vanilla, Vanilla New Game+ and Randomizer are
recorded in [Planned optional gameplay QoL](PLANNED-OPTIONAL-QOL.md). They are
not eligible for speedrun mode and are not part of the current release scope.
The list includes GBA-like physics, movement/weapon options, faster traversal,
revised item selection and shorter expansion pickup sequences.
