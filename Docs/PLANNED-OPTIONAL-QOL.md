# Planned optional gameplay QoL

Recorded September 19, 2026, at the project owner's request.

**Status: requested future work. This list does not claim implementation or inclusion in the current release.** Audit existing overlapping options before implementing; do not duplicate or silently change them.

## Scope and speedrun eligibility

- Optional settings for **Vanilla, Vanilla New Game+ and Randomizer**.
- **Not eligible for speedrun mode**, including its existing QoL category. The previously allowed Wall Jump/Space Jump assists are a separate policy; this request does not expand that allowlist.
- Boss Rush is outside the requested scope.
- Future implementation must keep Randomizer generation and tracker logic consistent with enabled movement, weapon and door rules.

## Movement and physics

- **GBA-like physics:** heavier physics closely resembling the GBA Metroid games.
- **RESPIN:** press Jump at any time during an ordinary fall to resume spinning.
- **BOMB SKIP:** hold Down to avoid bomb jumping.
- **SPEED BALL:** with Spring Ball equipped, run at full speed in Morph Ball form.
- **SPIN FALL:** hold Jump while falling from a ledge to start spinning automatically.
- Allow underwater wall jumping and bomb jumping.
- Add a toggle between Run and Walk.

## Faster traversal and pickups

- Faster doors, room transitions and item-acquisition sequences.
- **Elevators at twice the original Super Metroid speed.** This is the specific target for the faster-elevator request above, not a second stacking multiplier.
- Allow item message boxes to be skipped with a button press.
- Replace the normal expansion-tank fanfare with a short pickup SFX for Missile, Super Missile, Power Bomb, Energy and Reserve expansions.
- Keep the long normal fanfare for major equipment (beams, suits, etc.) and the first Missile, Super Missile and Power Bomb pickups, similar to Zero Mission / Fusion.

## Weapons and resources

- Adjust beam travel and firing speeds; increase auto-fire speed. Exact values remain to be defined.
- Charging the beam attracts enemy energy/ammo drops and charges slightly faster.
- Red missile doors open with **one Missile instead of five**, like Zero Mission / Fusion.
- Screw Attack destroys frozen enemies. **Credit: Adamf**, as supplied in the request; confirm the relevant implementation/provenance when integrating.
- Power Bombs reveal hidden tiles, like Zero Mission / Fusion.
- **Preserve Crystal Flash** as in the original game; the new options must not disable it.

## Item-selection controls

- Select Item cycles Missiles, Super Missiles and Grapple Beam.
- Remove X-Ray Scope and Power Bombs from that selection cycle.
- Give X-Ray Scope its own button.
- In Morph Ball form, pressing Cancel automatically highlights Power Bombs.
- Keep beams available through Cancel while Missiles or Super Missiles are selected.
- Preserve the existing LT/RT cycling preference where compatible; exact binding interaction is an implementation detail to validate later.

## Integration work to plan

Keep settings optional and persistent, show their speedrun eligibility clearly,
and prevent runs with these options from entering eligible speedrun categories.
Review randomizer logic and tracker assumptions for altered traversal, red doors,
underwater movement and resource collection. Exact tuning and control conflicts
remain future work, not new defaults or finalized behavior in this note.
