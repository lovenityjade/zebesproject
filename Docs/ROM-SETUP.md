# Player-supplied ROM and blocking startup

The game requires an unmodified, unheadered Japan/USA ROM:

- Suggested filename: `Super Metroid (Japan, USA) (En,Ja).sfc`
- Size: 3,145,728 bytes
- CRC32: `D63ED5F8`
- SHA-1: `da957f0d63d14cb441d215462904c4fa8519c613`

Filenames are not proof of compatibility. A renamed file with the exact contents
is accepted; modified/randomized, headered, truncated and other-region images
are rejected. Nothing is downloaded or searched for on the network.

At each normal startup the local `roms/` copy is read and checked. When absent
or invalid, an English ImGui setup screen replaces game startup. It displays
the required filename, CRC and size, offers a filesystem browser and a pasted
path field, and keeps **Confirm, copy ROM and continue** disabled until the
selection is verified. Keyboard/mouse and the existing ImGui controller
navigation are available. The native gameplay library is not loaded at this
point; no game state, saves, audio or ROM graphics are initialized.

After confirmation, the source is read and checked again. The game writes a
temporary sibling file, verifies it from disk, and then installs it under the
canonical local filename. An invalid previous local copy is preserved with an
`.invalid-<unique id>` suffix. The source is never moved or changed. Permission
or disk errors leave startup blocked with an actionable error. A valid existing
local copy is left alone. The next launch repeats validation; no cached success
flag can bypass it. The native ABI also rejects a wrong size/CRC before SnesInit.

## Nintendo asset distribution boundary

The public source snapshot and release packages exclude original ROM images,
pre-extracted tracker artwork, mixed room payloads and optional music recordings.
Credits, HUD, objective-menu graphics, tracker icons and the helmet are decoded
from the validated player's ROM. Room changes are reconstructed from original
room data plus reviewed VARIA edits. Private historical snapshots are not releases.

`Scripts/check-distribution.py` checks the known runtime boundaries and binds
reconstruction recipes to their reviewed hashes. Packaging and the pre-push hook
invoke it; configure `git config core.hooksPath .githooks` in a fresh checkout.
Source export and staged-file audits are separate mandatory release checks.

In an AppImage, ROM storage is `$XDG_DATA_HOME/zebesproject/roms` or
`~/.local/share/zebesproject/roms`. Other installations use their local `roms/`
directory. Every startup validates the actual file, including after an update.

## Validation

Linux Game, Editor and native library builds passed. On gaming-pc, isolated fixtures:

- `-SMRomSelfTest=<isolated-root>` passed: absent, valid, renamed, corrupted,
  changed-after-selection, headered, truncated and empty input; installation,
  same-file import, corruption after installation, preserved invalid backup,
  and an unwritable/invalid destination path.
- Real rendered missing-ROM and invalid-CRC setups: `-SMRomSetupTest`, with an isolation marker,
  captured the UI and reported `SM_ROM_GATE_TEST PASS`; native core, readiness
  and gameplay texture remained absent.
- Direct native ABI: absent/bad-CRC ROMs rejected without creating a save;
  valid ROM initialized successfully. Five runtime-extracted credit arrays
  matched their previous SHA-256 values exactly.
- Synthetic export tests passed: mandatory ROM exclusion, ignored legacy opt-in,
  documentation allowlist, stale-output rejection and preservation.

The interactive picker/import path has not yet had a manual player acceptance
run. No game was launched on the local desktop and no player save was modified.
