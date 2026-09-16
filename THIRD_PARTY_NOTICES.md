# Credits and third-party notices

Project-authored code and text documentation are MIT-licensed; see [LICENSE](LICENSE).
Artwork/audio/branding have separate terms in [ASSET_LICENSE.md](ASSET_LICENSE.md).
Neither notice relicenses third-party contributions.

The Zebes Project: TheLovenityJade and Sekailink (https://sekailink.com).
Super Metroid, its characters and original artwork belong to Nintendo.
No game ROM or Unreal Engine distribution is included in this source repository.

- Native C gameplay: snesrev/sm, pinned in `native-core/`; see `native-core/LICENSE.txt`.
- Disassembly research reference: strager/supermetroid, revision `b7785a19024454ef366500243648cb64e687164b`; not bundled.
- VARIA randomizer: theonlydude/RandomMetroidSolver (dude and flo), pinned runtime subset in `Randomizer/upstream/`; MIT, see its `LICENSE` and the file inventory in `Randomizer/upstream-provenance.json`.
- Dear ImGui: `Unreal/ThirdParty/ImGui/LICENSE.txt` (MIT).
- Montserrat: `Unreal/Content/UI/OFL.txt` (SIL Open Font License).
- Embedded CPython on Windows: Python Software Foundation and contributors,
  official Python 3.13.15 x64 embedded distribution. The runtime is downloaded
  separately and hash-checked; retain its complete `Runtime/Python/LICENSE.txt`
  in any package. It includes notices for components bundled by CPython.
- Windows native build: MinGW-w64/UCRT and GCC. Preserve the applicable runtime
  notices and GCC Runtime Library Exception with any shipped native DLL; the
  project's MIT license does not replace those terms.
- Tracker design reference: Cyb3R’s Super Metroid PopTracker pack (MIT notice retained in `Native/TrackerAssets-LICENSE.txt`). The pack and its extracted sprites are not bundled. Runtime icons are decoded from the player’s ROM.
- Adapted native credits and VARIA UI/world patches: `Native/Credits-LICENSE.txt`, `Native/VariaUI-LICENSE.txt`, `Native/WorldPatches-LICENSE.txt`.
- Application icons: project-owned startup logo, then original ROM-decoded helmet; see `Unreal/Content/Splash/README.md`.

These notices do not relicense third-party code or artwork. Unreal Engine is obtained separately under Epic's terms. Local reference art and the authored Chozo relic import retain their documented provenance under `Assets/ChozoRelic/`.

## Contributors and optional soundtrack

The expanded contributor tables are in the [README](README.md#credits--built-on-an-extraordinary-community).
They include the native runtime and disassembly authors, VARIA staff and credited
community contributors, UI patch authors, tracker references and middleware.
The original game staff credit roll is retained and decoded from the user's ROM.
Upstream contributor histories and source notices remain authoritative for the
full contribution history.

Optional remastered music: Jammin’ Sam Miller; JUD6MENT; NoNameSD; Blake Robinson /
The Synthetic Orchestra; The Noble Demon; GMB Sound Team; Pontus Hultgren Music;
VG Music Revisited; Dj @tomnium; Wingus Dingus. Intro voice: Luke Correia.
MSU integration references: DarkShock and Cubear. See
[recording provenance](Docs/REMASTERED-SOUNDTRACK.md). The refreshed Windows ALPHA-0.24 archive includes
the selected recordings and their credits; they retain their authors' terms,
independently of the project's code license. The Linux AppImage excludes them.
The separately supplied game-over recording is also excluded. The game falls
back to original ROM audio when no optional local recording is installed.

## Tracker compatibility and original assets

The tracker retains the documented PopTracker status color convention. Its
native marker rasterizer and state-sector selection are implemented in this
project; no PopTracker application source or GPL renderer is linked or bundled.
The upstream application's GPL notice is retained only as a research reference,
not as a license for project-authored code. Original game pixels, room streams,
HUD graphics and helmet artwork are reconstructed from the validated player ROM.
Reconstruction recipes and verification reports are in
`Docs/Releases/ALPHA-0.24/`. These notices do not grant rights to Nintendo material.

## Unreal Engine attribution

The Zebes Project uses Unreal® Engine. Unreal® is a trademark or registered
trademark of Epic Games, Inc. in the United States of America and elsewhere.
Unreal® Engine, Copyright 1998 – 2026, Epic Games, Inc. All rights reserved.

Linux packages include CPython 3.11, libffi, and the AppImage type-2 runtime.
Their notices accompany the package in `Licenses/`. The AppImage runtime is a
separate replaceable launcher; its source/build instructions are linked there.
Unreal's third-party notices accompany the packaged engine separately.
