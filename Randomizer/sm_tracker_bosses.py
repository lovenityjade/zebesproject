"""Combat markers, separate from the 100 item checks and their counters.

Coordinates use original room headers, including the native map's +1 Y.
Identity order is the sm_tracker_publish_bosses ABI; keep it stable.
"""
BOSSES = (
    ('Kraid', 'Kraid', 1, 1, 56, 20, 'Kraid'),
    ('Phantoon', 'Phantoon', 3, 1, 19, 20, 'WreckedShip'),
    ('Draygon', 'Draygon', 4, 1, 40, 11, 'EastMaridia'),
    ('Ridley', 'Ridley', 2, 1, 23, 18, 'LowerNorfair'),
    ('Mother Brain', 'MotherBrain', 5, 2, 15, 19, 'Tourian'),
    ('Spore Spawn', 'SporeSpawn', 1, 2, 22, 3, 'GreenPinkBrinstar'),
    ('Crocomire', 'Crocomire', 2, 2, 15, 11, 'Crocomire'),
    ('Botwoon', 'Botwoon', 4, 2, 25, 9, 'EastMaridia'),
    ('Golden Torizo', 'GoldenTorizo', 2, 4, 19, 17, 'LowerNorfair'),
    ('Bomb Torizo', None, 0, 4, 25, 7, 'Crateria'),
)


def evaluate_bosses(logic, graph, sm, root, maximum, inventory, config, topology):
    import copy
    from utils.parameters import infinity
    from rom.rom_patches import RomPatches
    sources = {loc.Name: loc for loc in logic.locations()}
    mini = topology.get('nativeMinimizer')
    if mini:
        from sm_minimizer import catalog
        retained = {catalog()['regions'][i] for i in mini['regions']}
    config = config or {}

    def query(name, item, limit):
        # Bomb Torizo shares the Bomb pickup's approach and needs no special
        # weapon to defeat. Never infer the hidden placed item's identity.
        loc = copy.deepcopy(sources['Bomb' if item is None else name])
        graph.getAvailableLocations([loc], sm, limit, root)
        reachable = bool(loc.difficulty.bool)
        difficulty = loc.difficulty.difficulty if reachable else None
        requirements = sorted(set(loc.difficulty.items)) if reachable else []
        techniques = sorted(set(loc.difficulty.knows)) if reachable else []
        owned = item is not None and sm.haveItem(item).bool
        try:
            # A victory opens its own exit (notably Draygon). This is a
            # hypothetical return check only, never an inventory grant.
            if item and not owned:
                sm.addItem(item)
            safe = reachable and difficulty <= limit and bool(graph.canAccess(sm, loc.accessPoint, root, limit))
        finally:
            if item and not owned:
                sm.removeItem(item)
        return dict(reachable=reachable, safe=safe, difficulty=difficulty,
                    requirements=requirements, techniques=techniques)

    result = []
    for index, (name, item, area, mask, x, y, region) in enumerate(BOSSES):
        defeated = bool(inventory['bosses'][area] & mask)
        enabled = not (mini and index >= 5 and region not in retained)
        if name == 'Mother Brain' and config.get('tourian') == 'Disabled':
            enabled = False
        if name == 'Bomb Torizo' and config.get('nativeObjectives', {}).get('flags', 0) & 4:
            enabled = False
        access, alt = query(name, item, maximum), query(name, item, infinity)
        if name == 'Bomb Torizo' and RomPatches.BombTorizoWake not in RomPatches.ActivePatches and not sm.haveItem('Bomb').bool:
            # The original statue wakes only with Bombs. Picking up an unknown
            # randomized item cannot be assumed to provide them.
            access.update(reachable=False, safe=False)
            alt.update(reachable=False, safe=False)
        state = 0 if defeated else 1 if access['safe'] else 4 if alt['reachable'] else 2
        reason = ('Defeated' if defeated else 'Accessible fight' if access['safe'] else
                  'Return not guaranteed' if access['reachable'] else
                  'Above configured difficulty' if alt['reachable'] else 'Blocked by current inventory or rules')
        if not enabled:
            state = 255
            reason = 'Encounter disabled or outside this world'
        result.append(dict(index=index, name=name, area=area, x=x, y=y,
                           enabled=enabled, defeated=defeated, state=state, reason=reason, **access))
    return result
