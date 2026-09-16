# Contributing

The project is in alpha. Start with a reproducible bug report or a focused
proposal. Keep the original game's identity, existing saves and independently
configured seed slots intact. Story, Boss Rush and multiworld are planned work,
not invitations to invent unapproved story canon or change seed logic casually.

Project-authored code contributions are accepted under the root MIT license.
Do not submit third-party code without its source and compatible license. Artwork
and audio require explicit provenance and permission; code licensing does not
apply to them automatically. Clearly identify AI-assisted/generated contributions.
Do not submit ROMs, extracted Nintendo media, player saves, account credentials,
or downloaded soundtrack packs.

Include what changed, why, the platform/toolchain and tests actually performed.
For randomizer changes include seed/settings, solver progression and an isolated
native-runtime check where relevant. For visuals include rendered evidence and
preserve original assets. Document unsupported or untested paths honestly.

The distribution guard is mandatory. A failing guard must be resolved through
asset migration and provenance review, never by deleting the check or disabling
Git hooks. Frozen versions receive only reviewed fixes with matching validation;
new features belong after the freeze.
