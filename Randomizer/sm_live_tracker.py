"""Live, spoiler-free VARIA queries, serialized by the embedded Python host.

One fresh subinterpreter per query: no globals or cached techniques leak between
slots or generation. Only effective rules and native inventory enter this API.
"""
import contextlib
import copy
import io
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent


def evaluate(request):
    sys.path.insert(0, str(ROOT / 'upstream'))
    from logic.logic import Logic
    from logic.smbool import SMBool
    from logic.smboolmanager import SMBoolManager
    from graph.graph import AccessGraphSolver
    from graph.graph_utils import vanillaTransitions, vanillaBossesTransitions, vanillaEscapeTransitions, GraphUtils
    from rom.flavor import RomFlavor
    from rom.rom_patches import RomPatches
    from utils.parameters import Knows, Settings, isKnows, infinity, medium, getDiffThreshold
    from utils.utils import PresetLoader
    from utils.objectives import Objectives
    from utils.doorsmanager import DoorsManager
    from rando.Items import ItemManager

    Logic.factory('vanilla', new=True)
    RomFlavor.factory(str(ROOT / 'upstream'))
    config = request.get('settings')
    randomized = request['randomized']
    if randomized:
        if not config or config['start'] not in GraphUtils.getStartAccessPointNames() or config['split'] not in ('Full','FullWithHUD','Major','Chozo','Scavenger'):
            raise ValueError('Unsupported tracker context')
        # Expanded values, not whichever preset happens to be installed later.
        for name, value in config['knows'].items():
            if not isKnows(name) or not hasattr(Knows, name):
                raise ValueError('Unknown technique: ' + name)
            setattr(Knows, name, SMBool(value['enabled'], value['difficulty'], [name]))
        Settings.hardRooms = copy.deepcopy(config['hardRooms'])
        Settings.hellRuns = copy.deepcopy(config['hellRuns'])
        Settings.bossesDifficulty = copy.deepcopy(config['bossesDifficulty'])
        # JSON object keys are strings; boss energy thresholds are numeric.
        for setting in Settings.bossesDifficulty.values():
            if isinstance(setting, dict) and 'Energy' in setting:
                setting['Energy'] = {float(k): v for k, v in setting['Energy'].items()}
        RomPatches.ActivePatches = list(config['logicPatches'])
        maximum = config['maximumDifficulty']
        topology = request['topology']
        from sm_topology import validate
        validate(topology)
        if 'doorColors' in topology:
            from sm_door_colors import validate as validate_doors
            validate_doors(dict(topology['doorColors'],doors=topology['doors']))
        for name, door in topology['doors'].items():
            DoorsManager.doors[name].setColor(door['color'])
            DoorsManager.doors[name].hidden = door['hidden']
    else:
        skill = request.get('skill', 'casual')
        if skill not in ('casual', 'regular', 'veteran'):
            raise ValueError('Unsupported vanilla tracker skill')
        PresetLoader.factory(str(ROOT/'upstream/standard_presets'/f'{skill}.json')).load()
        RomPatches.ActivePatches = []
        maximum = getDiffThreshold(medium)

    geometry = json.loads((ROOT/'native_locations.json').read_text())['locations']
    ordered = sorted(geometry, key=lambda loc: loc['address'])
    inventory = request['inventory']
    collected = inventory['collected']
    if len(collected) != 100 or any(type(v) not in (int, bool) or v not in (0, 1) for v in collected):
        raise ValueError('Invalid collection snapshot')
    collected_names = {loc['name'] for loc, flag in zip(ordered, collected) if flag}
    if inventory['bosses'][2]&1:collected_names.add('Ridley')
    hunt=config.get('scavenger') if randomized else None
    blocked_hunt=set()
    if hunt:
        from sm_scavenger import validate as validate_scavenger
        validate_scavenger(hunt)
        if config.get('nativeObjectives',{}).get('scavenger')!=hunt:raise ValueError('Tracker Scavenger contracts disagree')
        position=next((i for i,n in enumerate(hunt['locations']) if n not in collected_names),len(hunt['locations']))
        snapshot=request.get('objectiveState')
        if snapshot is not None:
            try:native_position=snapshot['values'][snapshot['goals'].index(16)][1]
            except (KeyError,ValueError,IndexError,TypeError):raise ValueError('Missing native hunt progress') from None
            if native_position!=position:raise ValueError('Scavenger progress disagrees with collection bits')
        blocked_hunt=set(hunt['locations'][position+1:])
    elif randomized and config['split']=='Scavenger':raise ValueError('Missing Scavenger order')
    from sm_objectives import configure_logic
    Objectives.startAP=config['start'] if randomized else 'Landing Site'
    configure_logic(config.get('nativeObjectives') if randomized else None,
                    lambda: collected_names)
    from sm_topology import validate,vanilla
    topology=topology if randomized else vanilla()
    graph = AccessGraphSolver(Logic.accessPoints(), validate(topology))
    start = config['start'] if randomized else 'Landing Site'
    # Ceres is a one-way prologue, never a destination to return to from Zebes.
    root = 'Landing Site' if start=='Ceres' else start
    Objectives.startAP = root
    Objectives.setGraph(graph, infinity)
    sm = SMBoolManager()
    inventory = request['inventory']
    for item in ItemManager.Items.values():
        if ((item.ItemBits and inventory['items'] & item.ItemBits) or
                (item.BeamBits and inventory['beams'] & item.BeamBits)):
            sm.addItem(item.Type)
    for name, value in [('ETank', max(0, (inventory['health'] - 99) // 100)),
                        ('Reserve', inventory['reserve'] // 100),
                        ('Missile', inventory['missiles'] // 5),
                        ('Super', inventory['supers'] // 5),
                        ('PowerBomb', inventory['powerBombs'] // 5)]:
        if not 0 <= value < 128:
            raise ValueError('Invalid native inventory capacity')
        for _ in range(value):
            sm.addItem(name)
    # SNES boss bits, including the minibosses used by VARIA traversal rules.
    for name, area, mask in [('Kraid',1,1), ('Phantoon',3,1), ('Draygon',4,1),
            ('Ridley',2,1), ('MotherBrain',5,2), ('SporeSpawn',1,2),
            ('Crocomire',2,2), ('Botwoon',4,2), ('GoldenTorizo',2,4)]:
        if inventory['bosses'][area] & mask:
            sm.addItem(name)
    doors = inventory['doors']
    for door in DoorsManager.doors.values():
        if door.id is not None and 0 <= door.id < 512 and doors[door.id // 8] & (1 << (door.id % 8)):
            door.setColor('blue')

    source = {loc.Address: loc for loc in Logic.locations() if not loc.isBoss()}

    def query(limit):
        locations = [copy.deepcopy(source[entry['address']]) for entry in ordered]
        # No itemName assignment. In particular no seed placement enters here.
        graph.getAvailableLocations(locations, sm, limit, root)
        result = []
        for loc in locations:
            loc.evalPostAvailable(sm, 'tracker')
            if loc.Name in blocked_hunt:loc.difficulty=SMBool(False)
            reachable = loc.difficulty.bool
            if reachable:
                loc.comeBack = graph.canAccess(sm, loc.accessPoint, root, limit)
            safe = reachable and not loc.mayNotComeback and loc.comeBack and loc.difficulty.difficulty <= limit
            result.append(dict(reachable=bool(reachable), safe=bool(safe),
                difficulty=loc.difficulty.difficulty if reachable else None,
                techniques=sorted(set(loc.difficulty.knows)) if reachable else [],
                requirements=sorted(set(loc.difficulty.items)) if reachable else []))
        return result

    normal, alternative = query(maximum), query(infinity)
    membership=topology.get('nativeMinimizer',{}).get('checks',[1]*100)
    checks = []
    collected = inventory['collected']
    if len(collected) != 100:
        raise ValueError('Invalid collection snapshot')
    for i, (native, access, alt) in enumerate(zip(ordered, normal, alternative)):
        state = 0 if collected[i] else 1 if access['safe'] else 4 if alt['reachable'] else 2
        reason = ('Collected' if collected[i] else 'Accessible' if access['safe'] else
                  'Return not guaranteed' if access['reachable'] else
                  'Above configured difficulty' if alt['reachable'] else 'Blocked by current inventory or rules')
        if native['name'] in blocked_hunt and not collected[i]:reason='Later in the Scavenger hunt order'
        if not membership[i]:
            state=0;reason='Excluded from this Minimizer world'
            access=dict(reachable=False,safe=False,difficulty=None,techniques=[],requirements=[])
        checks.append(dict(enabled=bool(membership[i]), index=i, address=native['address'], area=native['area'],
            x=native['mapX'], y=native['mapY'], name=native['name'], state=state,
            reason=reason, **access))
    from sm_tracker_bosses import evaluate_bosses
    bosses = evaluate_bosses(Logic, graph, sm, root, maximum, inventory, config, topology)
    from sm_objectives import tracker_progress
    objectives = tracker_progress(config.get('nativeObjectives') if randomized else None,
                                  request.get('objectiveState'), sm, root, maximum)
    return dict(ok=True, maximumDifficulty=maximum, startAccessPoint=start, returnAccessPoint=root, topologyMode=topology['mode'], bossConnections=topology['bossPairs'], checks=checks, bosses=bosses, objectives=objectives)


def evaluate_json(request_json, _attempt=0):
    try:
        with contextlib.redirect_stdout(io.StringIO()), contextlib.redirect_stderr(io.StringIO()):
            result = evaluate(json.loads(request_json))
    except Exception as ex:
        result = dict(ok=False, error=f'{type(ex).__name__}: {ex}')
    return json.dumps(result, separators=(',', ':'))
