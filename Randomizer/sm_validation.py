"""Replay serialized native placements through a fresh solver and inventory.

This proves the logical route for the declared rules, not a complete native
playthrough. Activation hooks listed in the manifest still need gameplay QA.
"""
from collections import Counter
import copy
from pathlib import Path


def verify_progression(placements, skill, logic_patches, rules=None, context=None):
    from logic.logic import Logic
    from rando.Items import ItemManager
    from rom.rom_patches import RomPatches
    from graph.graph import AccessGraphSolver
    from graph.graph_utils import vanillaTransitions, vanillaBossesTransitions, vanillaEscapeTransitions
    from solver.randoSolver import RandoSolver
    from solver.runtimeLimiter import RuntimeLimiter
    from utils.objectives import Objectives
    from utils.utils import PresetLoader
    from utils.parameters import infinity, medium, getDiffThreshold
    from utils.parameters import text2diff
    difficulty_limit = getDiffThreshold(text2diff[rules['options']['maxDifficulty']]) if rules else getDiffThreshold(medium)
    start=(context or {}).get('start','Landing Site')
    if rules and rules['relicHunt']['enabled']:
        from sm_relics import register_relic
        register_relic()

    Logic.factory('vanilla', new=True)
    from rom.flavor import RomFlavor
    RomFlavor.factory(str(Path(__file__).parent/'upstream'))
    RomPatches.ActivePatches = list(logic_patches)
    from utils.doorsmanager import DoorsManager
    DoorsManager.setDoorsColor()
    from sm_door_colors import apply_context
    apply_context(context)
    from sm_initial_doors import apply_context as apply_initial_doors
    apply_initial_doors(context)
    PresetLoader.factory(rules['preset'] if rules else str(Path(__file__).parent/'upstream/standard_presets'/f'{skill}.json')).load()
    from sm_objectives import configure_logic
    objective_contract = (context or {}).get('objectives')
    Objectives.startAP=start
    configure_logic(objective_contract,
                    lambda: {loc.Name for loc in solver.container.visitedLocations()})
    locations = copy.deepcopy(Logic.locations())
    for loc in locations:
        loc.AccessFrom = dict(loc.AccessFrom)  # solver endgame closures must not mutate shared location classes
    by_address = {loc.Address: loc for loc in locations if not loc.isBoss()}
    items_by_code = {item.Code: item.Type for item in ItemManager.Items.values()
                     if item.Code is not None and item.Category not in ('Nothing', 'Boss', 'MiniBoss')}
    decoded = {}
    if len(placements) != 100 or len({p['address'] for p in placements}) != 100:
        raise ValueError('Expected 100 unique placement addresses')
    for p in placements:
        loc = by_address.get(p['address'])
        if loc is None or loc.Name != p['location']:
            raise ValueError('Spoiler location/address mismatch')
        visibility=p['visibility']
        if visibility!=loc.Visibility and not (rules and rules['options'].get('hideItems')=='on' and loc.CanHidden and loc.Visibility=='Visible' and visibility=='Hidden'):
            raise ValueError('Spoiler visibility differs from the native carrier rules')
        offset = {'Visible': 0, 'Chozo': 84, 'Hidden': 168}[visibility]
        kind=p.get('kind',0)
        if kind not in (0,1,2):raise ValueError('Unknown native item kind')
        if kind and p['plm']-offset != ItemManager.Items['Reserve'].Code:raise ValueError('Custom item carrier mismatch')
        item = 'ChozoRelic' if kind==1 else p['item'] if kind==2 and p['item'] in ('Nothing','NoEnergy') else items_by_code.get(p['plm'] - offset)
        if item is None or item != p['item']:
            raise ValueError('Spoiler item does not match the actual native PLM word')
        loc.itemName = item
        decoded[loc.Name] = item
    for loc in locations:
        if loc.isBoss():
            loc.itemName = loc.BossItemType
        loc.difficulty = None
    class BoundedGraph(AccessGraphSolver):
        def getAvailableAccessPoints(self, rootNode, smbm, maxDiff, item=None):
            return super().getAvailableAccessPoints(rootNode, smbm, min(maxDiff, difficulty_limit), item)
        def getAvailableLocations(self, locations, smbm, maxDiff, rootNode='Landing Site'):
            return super().getAvailableLocations(locations, smbm, min(maxDiff, difficulty_limit), rootNode)
    class BoundedSolver(RandoSolver):
        def computeLocationsDifficulty(self, locations, phase='major', startDiff=None):
            super().computeLocationsDifficulty(locations, phase, startDiff)
            from logic.smbool import smboolFalse
            for loc in locations:
                if loc.difficulty and loc.difficulty.difficulty > difficulty_limit:
                    loc.difficulty = smboolFalse
    from sm_topology import from_context,validate
    topology=from_context(context)
    mini=topology.get('nativeMinimizer')
    retained=set(mini['locations']) if mini else set(decoded)
    if mini:
        if any(decoded[name] not in ('Nothing','NoEnergy') for name in set(decoded)-retained):raise ValueError('Excluded check contains an item')
        from graph.graph_utils import graphAreas
        locations=[loc for loc in locations if loc.Name in retained or loc.isBoss() and (loc.Name in ('Kraid','Phantoon','Draygon','Ridley','Mother Brain') or graphAreas.index(loc.GraphArea) in mini['regions'])]
    graph = BoundedGraph(Logic.accessPoints(), validate(topology))
    Objectives.startAP = start
    Objectives.setGraph(graph, difficulty_limit)
    hunt=(context or {}).get('scavenger')
    solver = BoundedSolver('Scavenger' if hunt else 'Full', start, graph, locations)
    if hunt:
        from sm_scavenger import validate as validate_scavenger
        validate_scavenger(hunt)
        solver.romConf.masterMajorsSplit='Scavenger'
        by_name={loc.Name:loc for loc in solver.locations}
        solver.romConf.scavengerOrder=[by_name[name] for name in hunt['locations']]
        Objectives.goals['finish scavenger hunt'].setClearFunc(solver.scavengerHuntComplete)
    solver.romConf.tourian=(context or {}).get('tourian','Vanilla')
    solver.romConf.escapeTransition = [tuple(p) for p in topology['escapePairs']]
    solver.runtimeLimiter = RuntimeLimiter(30)
    solver.smbm.createKnowsFunctions()
    difficulty, items_ok = solver.computeDifficulty()
    visited = solver.container.visitedLocations()
    # Keep the historical item-only progressionLog stable. Source objective
    # steps carry separate visits (fish, robots, animals, exploration, etc.)
    # which must not disappear when auditing an objective-dependent route.
    objective_steps = []
    step_inventory = Counter()
    collected_checks = set()
    completed_goals = []
    quota_step = None
    for route_step, step in enumerate(solver.container.steps, 1):
        if hasattr(step, 'objectiveName'):
            entry = step.dump()
            completed_goals.append(step.objectiveName)
            entry.update(routeStep=route_step, afterLocationStep=sum(step_inventory.values()),
                         inventoryBefore=dict(sorted(step_inventory.items())),
                         collectedChecksBefore=sorted(collected_checks),
                         completedCount=len(completed_goals))
            for path in entry['paths']:
                if not path['pdiff'][0] or path['pdiff'][1] > difficulty_limit:
                    raise ValueError('Objective route exceeds configured difficulty')
            objective_steps.append(entry)
            if quota_step is None and len(completed_goals) >= Objectives.nbRequiredGoals:
                quota_step = route_step
        else:
            loc = step.location
            step_inventory[loc.itemName] += 1
            if loc.Name in decoded:
                collected_checks.add(loc.Name)
            if loc.itemName == 'MotherBrain' and quota_step is None:
                raise ValueError('Solver entered Tourian before completing required objectives')
    objective_verification = dict(names=[g.name for g in Objectives.activeGoals],
        required=Objectives.nbRequiredGoals, completed=completed_goals,
        quotaReached=quota_step is not None, quotaRouteStep=quota_step,
        progressionLog=objective_steps,
        evidence='VARIA solver feasibility and objective visits, not native completion events')
    inventory = Counter()
    progression = []
    relic=(rules or {}).get('relicHunt',{})
    relic_completion=None
    from logic.smboolmanager import SMBoolManager
    route_inventory=SMBoolManager()
    for index, loc in enumerate(visited):
        if not loc.difficulty.bool:
            raise ValueError('Solver visited an inaccessible location')
        if not loc.isBoss() and loc.itemName != 'Gunship' and decoded.get(loc.Name) != loc.itemName:
            raise ValueError('Progression and spoiler disagree')
        path = getattr(loc, 'path', None)
        progression.append(dict(step=index+1, location=loc.Name, item=loc.itemName,
                                inventoryBefore=dict(sorted(inventory.items())),
                                accessPoint=getattr(loc, 'accessPoint', None),
                                path=[ap.Name for ap in path] if path else [],
                                difficulty=loc.difficulty.difficulty,
                                techniques=sorted(set(loc.difficulty.knows)),
                                requirements=sorted(set(loc.difficulty.items))))
        inventory[loc.itemName] += 1
        if loc.itemName!='Gunship':route_inventory.addItem(loc.itemName)
        if relic.get('enabled') and relic_completion is None and inventory['ChozoRelic']>=relic['required'] and loc.itemName!='Gunship':
            # Verify that the quota can be taken home with the inventory at
            # this exact step, before any later progression items or bosses.
            exit_check=loc.PostAvailable(route_inventory) if loc.PostAvailable else True
            if exit_check and (exit_check is True or exit_check.difficulty<=difficulty_limit) and graph.canAccess(route_inventory,loc.accessPoint,'Landing Site',difficulty_limit):
                relic_completion=dict(step=index+1,location=loc.Name,collected=inventory['ChozoRelic'],
                    required=relic['required'],returnTo='Landing Site',inventory=dict(sorted(inventory.items())),
                    motherBrainRequired=False,returnVerified=True)
    hunt_route=[loc.Name for loc in visited if hunt and loc.Name in hunt['locations']]
    if hunt and hunt_route!=hunt['locations']:
        raise ValueError('Solver did not collect Scavenger checks in the source-written order')
    item_visits = {loc.Name for loc in visited if loc.Name in decoded}
    return dict(solver='VARIA RandoSolver, fresh graph and inventory',
                topologyMode=topology['mode'],bossConnections=topology['bossPairs'],areaConnections=topology['areaPairs'],
                difficulty=difficulty, maximumDifficulty=difficulty_limit, allItemsReachable=bool(items_ok and item_visits==retained),
                reachableItemCount=len(item_visits), retainedItemCount=len(retained), motherBrainDefeated='MotherBrain' in inventory,
                escapeToShip='Gunship' in inventory,
                completionMode='chozo-relic-hunt' if relic.get('enabled') else 'objectives-escape' if (context or {}).get('tourian')=='Disabled' else 'mother-brain',
                relicCompletion=relic_completion,
                objectiveVerification=objective_verification,
                scavengerVerification=dict(order=hunt_route,ordered=True) if hunt else None,
                completionVerified=bool(relic_completion) if relic.get('enabled') else 'Gunship' in inventory and (quota_step is not None if (context or {}).get('tourian')=='Disabled' else 'MotherBrain' in inventory),
                progressionLog=progression,
                unavailable=sorted(retained-item_visits), excluded=sorted(set(decoded)-retained))
