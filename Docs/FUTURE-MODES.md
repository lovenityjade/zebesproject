# Future modes and multiworld

Recorded September 16, 2026 at the user's request.

Status: planned additions. This document records the requested scope; it does
not mark these features implemented or start their implementation. Vanilla and
Randomized remain the current experiences to test and refine.

## Boss Rush

- A timed boss-rush challenge.
- Each boss is fought with the minimum equipment needed to defeat it.
- A dedicated leaderboard.

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

No implementation order or release date has been set.
