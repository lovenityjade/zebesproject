# Independent Vanilla / Randomized save slots

Implemented September 15, 2026. The original **SAMUS DATA** and **OPTION MODE**
menus remain the game menus, with their original ROM tiles, palettes and cursor.

## Player flow

Choose A, B or C, then use **MODE VANILLA / MODE RANDOMIZED** in OPTION MODE.
Left/Right, A or Start toggles an empty slot. Up/Down navigates the original rows.

| Slot | Start Game | Randomizer Options | Generate Game |
| --- | --- | --- | --- |
| New Vanilla | Enabled | Disabled | Disabled |
| New Randomized | Disabled | Enabled | Enabled |
| Generating | Disabled | Disabled | Disabled |
| Generated Randomized | Enabled | Disabled | Disabled |
| Existing Vanilla | Enabled | Disabled | Disabled |

**Randomizer Options** opens the existing English settings interface for that
specific slot. Seed number, skill and progression are persisted to its pending
request. **Return to game menu** returns to OPTION MODE. Generation itself stays
in the native game menu.

A generated slot is saved immediately at Landing Site, with native inventory,
location and checksums, before Start Game is confirmed. Subsequent progress still
uses the game's save stations. An existing or generated slot cannot change mode;
use native **Data Clear** first if you want to replace that game.

The randomized badge is the original menu star tile **0x5A**, from ROM
**8E:8B40**, with its original pixels and palette. The same tile appears beside
Randomized in OPTION MODE and beside the corresponding A/B/C entry in SAMUS DATA.
Pending randomized slots also carry it; they still say NO DATA until generated.
No generated illustration or replacement menu asset was added.

![Vanilla options](vanilla-options.png)
![Generated randomized slot](generated-1.png)
![A Vanilla, B and C Randomized](mixed-file-select.png)

Randomized slots also receive [complete original maps, including secret passages](../FullMaps/README.md), while retaining real exploration history.

## Storage and activation

```text
Saved/SM/Profiles/<bank-guid>/
  profile.json                     # schema 3; ordered references for A/B/C
  bank.sram.dat                    # authoritative 8192-byte native SRAM
  bank.transaction.json            # only while publishing/recovering an operation
  Achievements.ini                # existing bank-level milestones
  slots/<slot-manifest-guid>/
    profile.json                   # existing validated schema 1/2 metadata
    seed.json                      # generated seed + complete tracker contract
```

Nested manifests use the existing schema 1/2 reader and seed validation. Their
historical per-profile SRAM paths are not the bank's live SRAM. Each slot owns
its mode, request, seed, fingerprint and tracker manifest. Native item/boss/door/map
progress stays in the corresponding original SRAM slot. Achievements remain at
bank scope; they are not new per-seed tracker progress.

Selecting an actual file binds its item table to the in-memory ROM. Vanilla
restores the original 100 item PLMs. File-select preview reads do **not** switch
seeds. Tracker session identity advances on real activation/load, and its current
fingerprint follows the selected slot. The source ROM is never written.

A completed generation is first prepared in an unreferenced child manifest.
A journal then publishes the child reference and initial SRAM together. Copy and
Clear use the same journal so the seed identity follows the native operation.
On interrupted publication, loading the bank replays the journal before core
initialization. Merely listing saves never replays a live bank's journal. This is
process-interruption recovery, not a claim of hardware power-loss durability.

Copy creates a distinct manifest reference with the source seed. Clear resets
only the target to Vanilla. Old manifests are retained as unreferenced files.
Historical profiles are imported into a new bank without deleting their source;
played slots retain their seed, and empty slots remain configurable. An unstarted
historical seed/request is retained in A. The last activated bank ID is persisted
in Presentation.ini and restored on subsequent normal launches.

## Validation

Tests run **headlessly on gaming-pc**, in `/tmp/sm-native-generation-20260914`,
using separate SRAM and profile directories. The installed game and user saves
are not test targets.

- `Scripts/test-native-slots.py`: real native menu navigation, both mode toggles,
  disabled actions, settings callback, distinct seeds in B/C, immediate native
  save, shutdown/reload, Landing Site with 99 health, locked generated mode,
  injected save failure and retry, actual Copy/Clear confirmation menus, original
  100 Vanilla PLMs restored, and unchanged source ROM hash.
- `SMProfileSelfTest`: actual Unreal profile implementation, run from the game
  binary with `-nullrhi -nosound -unattended -SMProfileSelfTest=<isolated-root>`.
  Checks persisted independent requests, seed/tracker manifests, publication,
  copy/clear ownership, journal recovery, read-only library enumeration and
  rejection of invalid generation data. Requires `ISOLATED_TEST_DIRECTORY` and
  cannot run in Shipping builds.
- Native captures above were visually inspected. Native and Unreal builds pass.

Evidence: [native report](verification.json), [Unreal profile result](profile-verification.txt),
[build evidence](build-evidence.txt). These checks cover the native menus and
Unreal persistence separately; an interactive pass through the ImGui settings
and gamepad focus is still useful and has not been claimed as tested.
