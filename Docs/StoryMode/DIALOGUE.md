# Dialogue foundation

Implemented from the approved [dialogue lab](../../Labs/Dialogue/README.md).
This is an inactive presentation API. There are no authored scenes, automatic
triggers, demonstration conversations, new menu entries, or changes to the
availability of Story Mode.

## Appearance and assets

- Full-width bottom band, 52 native pixels high (400x224 or 256x224).
- 52x52 portrait frame, four-pixel gap, rectangular text frame.
- Two rows of native 8x8 text, no speaker-name label.
- Reveal at 32 ms per character; original yellow down arrow blinks every 500 ms
  once the current page is revealed.
- Frame, alphabet, digits, punctuation and arrow decoded in memory from the
  player's verified ROM. No extracted game artwork is shipped or written to disk.
- Optional caller-supplied portrait, centered and fitted inside 32x32 without
  changing aspect ratio. Use a nearest-filtered pixel-art texture. No default
  portrait or replacement artwork is added.
- Drawn after the game's atmosphere material, keeping the text and frame crisp.
  The build-version label is hidden while the dialogue occupies the bottom band.

Frame source: B6:E000 pause BG2 tilemap, B6:8000 graphics, B6:F000 palette.
The eight source crops match the accepted lab. Arrow source: B6:C000 OBJ tile
9D, palette colors 176–191, mirrored into the original 15-pixel arrow. Letters
use pause tiles 30–49; digits use the HUD font at 9A:B200. Additional native
punctuation: dash, apostrophe, exclamation mark, full stop.

## Unreal scripting API

Available on `ASMHUD`, under Blueprint category **Story | Dialogue**:

- `ShowDialogue(Text, Portrait)` → bool. Opens only during active gameplay,
  without another dialogue, native message, pause, system menu, teleport menu,
  startup warning or run recap. Returns false on unavailable assets/API,
  unsupported text, invalid length or an already active dialogue.
- `IsDialogueOpen()` → bool.
- `CloseDialogue()` cancels an active dialogue; otherwise does nothing.
- `OnDialogueClosed(Completed)` event: true only after confirming the final
  page, false on explicit cancellation. State is released before this callback,
  so a script can open the next speaker's dialogue from it.

The caller owns the dialogue text, speaker portrait and what happens afterwards.
One call can contain several pages. Spaces wrap at word boundaries, long words
split safely, and explicit newlines are preserved. Lowercase normalizes to the
original uppercase alphabet. Maximum 4096 input bytes and 512 lines. Supported
characters: A–Z, a–z, 0–9, spaces, CR/LF, `-`, `'`, `!`, `.`. Other characters are
rejected rather than silently disappearing; localization/additional glyphs are
outside this foundation.

Advance with the configured jump button, Start, or Space. A press during reveal
finishes the current page; a fresh press advances or closes. Holding a button
cannot skip pages. Opening and closing input are fenced so the trigger cannot
skip text and the last confirm cannot become a jump, shot or pause.

Gameplay simulation and game audio are suspended while the box is active. No
native frames or gameplay timers advance. The same frozen game image remains
under the dialogue. Closing restores the existing pause/audio state; gameplay
resumes after the controls are released. There is no voice or sound playback
added by this module.

## Native presentation API

`Native/sm_dialogue.h` exposes open/tick/close, state queries, and a BGRA pixel
buffer (`width * 52 * 4`). The native module does not consume gameplay input,
change RAM, write saves, or trigger story events. Unreal owns modal input,
portrait presentation and completion callbacks. ROM initialization loads the
assets; shutdown unloads the dialogue state. Reset-to-title cancels the box.

## Validation — gaming-pc

- `Scripts/test-dialogue-native.py`: passed for 400- and 256-pixel widths.
- Validated reveal, pagination, held/opening input, natural completion versus
  cancellation, long words, unsupported/oversized text, line overflow, bad delta
  values, hiding and teardown.
- Simulation RAM and native frame count unchanged throughout dialogue operations.
- ROM SHA-256 unchanged; isolated test SRAM only; zero emulated CPU opcodes.
- Native render captures visually inspected against the approved layout.
- Unreal Linux Development build succeeded, including UHT/Blueprint declarations.
- `git diff --check` passed.

Private evidence: `Build/Dialogue/`. This was native rendered validation and an
Unreal build, not a manual in-game dialogue playthrough. No game was launched on
the user's local desktop and no release download was replaced.
