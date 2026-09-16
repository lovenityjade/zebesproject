# Original and Remastered soundtrack

Settings → Audio → Soundtrack selects **Original** or **Remastered**, independently of save slots and randomizer settings. The preference is saved in Presentation.ini. Original is the default for new installations. Remastered replaces music commands, retaining the native SPC sound-effect engine. Missing, malformed or unreadable music falls back to the corresponding Original cue. No ROM patch is applied.

## Approved listening selection

The exact selection is recorded in `Config/Soundtracks/remastered.json` with SHA-256 fingerprints. It includes all 30 standard cues plus intro voice files 100 and 101:

- **Jammin’ Sam Miller / NoNameSD**: 2, 5, 6, 7, 9, 19, 21.
- **JUD6MENT, Orchestral v5 (January 2024)**: the remaining cues.
- **28 uses JUD6MENT's alternative version**, not the main version.

## Installation

The refreshed **Windows ALPHA-0.24** ZIP includes the randomizer crash fix and all
32 selected Remastered recordings. Extract the complete archive and launch the
root `SMUnreal.exe`; no separate hotfix or soundtrack download is required.
Select **Settings → Audio → Soundtrack → Remastered**. Original remains the
default for a new installation.

Files are stored at `Soundtracks/Remastered/zebes-N.pcm` beside the root launcher,
with `manifest.json` and `CREDITS.md`. Existing saves/settings are not part of the
archive. If adding recordings while the game is already running, switch to
Original and then back to Remastered to reload the current cue.

The **Linux AppImage** does not include these recordings. A separately installed
pack belongs in `Soundtracks/Remastered` beneath its persistent `SM_USER_DATA`
directory. Development folder installs use the project root. The recordings
remain excluded from Git; their authors' terms are separate from the code license.

## Credits

The native credit roll includes a Remastered soundtrack section after the original/VARIA credits and before gameplay statistics:

| Contribution | Credit |
| --- | --- |
| Original Super Metroid soundtrack | Nintendo's original credits remain in the roll |
| Audio restoration | Jammin’ Sam Miller |
| Orchestral pack compilation / conversion | JUD6MENT |
| Restored pack MSU conversion | NoNameSD |
| Arrangements represented by the selected orchestral pack's published credits | Blake Robinson / The Synthetic Orchestra; The Noble Demon; GMB Sound Team; Pontus Hultgren Music; VG Music Revisited; Dj @tomnium; Wingus Dingus |
| Intro voice performance | Luke Correia |
| MSU music cue integration reference | DarkShock; Cubear (intro update) |

`DJ ATOMNIUM` is used in the pixel-font roll because that font does not provide an at-sign; the artist's name is preserved as **Dj @tomnium** here.

Primary pack/creator sources:

- [JUD6MENT's showcase and individual arrangement credits](https://www.youtube.com/watch?v=wsOocVDTnpI). The complete description is retained locally in `.tmp/msu-audition/jud6ment-video-description.txt`.
- [Super Metroid MSU pack catalog and conversion credits](https://www.zeldix.net/t1444-super-metroid).
- [Restored pack by NoNameSD](https://drive.google.com/drive/folders/1-6ZgNzXj331gWxIKm8r0h_qzkFUht5DU).
- [DarkShock's original integration](https://github.com/mlarouche/SuperMetroid-MSU1).

## Runtime details

The native port translates the same bank/command pairs used by the MSU patch, including the two voice banks from Cubear's 2024 update. Command 4 keeps the original SPC ambience. The queue's original delays still control item fanfares, doors, bosses, death and cinematics. PCM mixes into the same 44.1 kHz stereo output before Unreal's master volume/pause controls.

Only header metadata is loaded when changing tracks; playback streams bounded blocks. Loop points are honored. Out-of-range loop headers restart at zero, matching bsnes's fallback. Fanfares, death, ending and spoken intro cues play once. Repeated requests for a currently selected cue do not restart it. Changing the soundtrack reissues the current cue; it restarts that arrangement rather than attempting to align arrangements of different lengths.

Credit preview owns a separate music stream and leaves the gameplay stream frozen. Closing the preview resumes the gameplay stream from its previous sample position. Reset and profile changes close old streams. The original ROM remains mandatory for both soundtrack choices.

## Validation (2026-09-15)

- Native C library, Unreal standalone game and Unreal Editor module built successfully.
- Isolated **gaming-pc** unit checks: all 25 mapping banks, signed PCM, clipping, loop boundaries, one-shot EOF, duplicate requests, ambience, missing/malformed files, live mode switch and independent preview cursor.
- Real native frame run: all 32 selected cues dispatched through the game's own music queue; Original/Remastered audio captured; live mode change preserves game RAM/frame clock; native SPC effects remain mixed; missing-directory fallback succeeds; shutdown releases playback.
- Credits preview leaves gameplay RAM and music cursor unchanged. Original ROM hash unchanged; zero emulated CPU opcodes.
- Unreal Audio menu screenshot and three native music-credit screenshots visually reviewed. Local game not launched.
- Local runtime installed and this workspace's next-launch preference set to Remastered. Original settings/library backed up under `.tmp/soundtrack`. New installations still default to Original.

Reproduce focused checks on the isolated test host with `Scripts/test-soundtrack.py` and `Scripts/test-soundtrack-native.py` (the latter uses the existing native fixture). Evidence is under `.tmp/soundtrack/proofs`. These checks cover routing/playback behavior, not a full playthrough or a subjective listening pass over every arrangement.
