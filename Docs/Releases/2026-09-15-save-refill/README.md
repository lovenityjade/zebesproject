# The Zebes Project — Save Refill release

This checkpoint precedes the complete VARIA settings editor work.

Settings → Quality of Life → **Refill energy and ammo when saving**.

The option is off by default, persisted in `SM/Presentation.ini` under `[QualityOfLife] RefillBeforeSave`, and applies to Vanilla and Randomized sessions, including existing saves. On accepting a save-station prompt, current energy, reserves, Missiles, Super Missiles and Power Bombs are set to their existing maximums **before** native SRAM capture. It grants no capacity or equipment. Refusing the prompt leaves supplies unchanged. Disabling the option restores the original station behavior. Samus's ship already refills supplies through its original routine.

Only the accepted station save path is hooked. Flushing SRAM on exit, copying a slot, generating a new file, and writing metadata do not invoke a refill. This is a native QoL implementation, not execution of an IPS patch. Unlike upstream's animated refill-before-prompt patch, this release refills immediately after confirmation; the distinction is intentional for this requested save-confirmation feature.

Verification targets: actual native station PLM collision and Yes/No interaction on an isolated gaming-pc fixture, option on/off, Vanilla/Randomized and saved inventory on reload. The fixture positions Samus above the actual Crateria station and clears its post-load lockout to represent a fresh room entry. It does not call a save/grant function directly. Rendering and configuration binding are checked separately in Unreal.

The user's local game is not restarted by packaging this release. Runtime files for future launches are staged separately; the release directory retains a frozen executable, library, sources and checksums. The private runtime contains the existing local ROM and must not be treated as a public redistribution artifact.

All six native cases passed (`verification.json`), including saved-inventory reload. Native, Game and Editor builds passed. The offscreen Unreal capture `refill-menu.png` was visually inspected; the checkbox is present and off by default (`SM_REFILL_MENU configured=0 native=0`).

A second Unreal boot with the persisted setting enabled reported `SM_REFILL_MENU configured=1 native=1`; the isolated test configuration was restored afterward.
