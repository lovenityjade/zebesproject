# Escape memories

Implemented for the common native Zebes destruction event, across Vanilla, Vanilla+, Story and Randomizer, including the tablet escape. Boss Rush and Ceres are excluded. Story uses this shared path when its gameplay mode is enabled; this does not enable the unfinished Story campaign.

Seven automatic exchanges begin at 8, 28, 48, 68, 88, 108 and 128 seconds of playable escape time. Each lasts 5.8 seconds: the remembered speaker for 3.15 seconds, then Samus for 2.65 seconds. The final reply closes at 133.8 seconds. Native map/pause, door loads and item messages suspend the readable sequence clock. Reaching the ship, dying or resetting cancels it. No escape duration, collision, controls or music changes.

The existing ROM-derived pause frame and glyphs draw the dialogue at native resolution. Approved 32x32 portraits from Assets/StoryMode/Portraits/v1 are copied unchanged to the staged Story/Portraits directory, with Zero Suit Samus for replies. No speaker labels. Exchanges alternate top/bottom starting at the top edge over the energy/missile HUD, as requested in the follow-up. Open in 180 ms, close in 220 ms using an eased centred crop of the frame and portrait; pixels are never stretched. Both animations are included in the 5.8 seconds. The box stays open while switching speaker to Samus and closes fully between exchanges.

Text reveals quickly, then remains readable. Automatic pagination handles 4:3 and French text. Confirm/fire/jump inputs cannot skip or dismiss a memory, and the dialogue never pauses simulation or audio. A gently eased, 42% desaturation with a slight lift affects the world only. HUD, escape timer and dialogue stay separate; normal colours return between memories. This presentation is not tied to the optional atmosphere toggle.

English is the supplied dialogue with spelling/punctuation corrections only. French Canadian translations are authored in Localization/fr-CA.tsv, compiled through the existing localization pipeline.

## Validation

Build and tests run on gaming-pc in the isolated release-alpha025 checkout. The original manual-dialogue regression remains passing, including input fence and no simulation writes. The new native test checks all 14 lines in both languages at both widths (56 combinations), automatic page reveal and no RAM/frame mutations. The rendered escape fixture validates the complete schedule and portrait switching with real native gameplay stepping; it is not a human escape playthrough or a completed Story campaign test.

## Punctuation correction

The pause alphabet tiles 4A/4B/4C are period/question/exclamation. The original mapping incorrectly used 4B for apostrophe and a border tile for period. Dialogue now uses those native punctuation tiles and the item-message apostrophe FD; straight and curly apostrophes render identically. Pixel-level regression checks assert the baseline dot, top apostrophe and distinct question mark at both widths. The upper panel starts at native Y=4, over the HUD; its lower edge also covers the countdown completely instead of leaving clipped digits beneath the frame.
