"""Typed objective contract from VARIA's effective data writers.

The native v1 evaluator supports full original/area layouts and ordinary Tourian.
Generation serializes this contract for full-layout ordinary-Tourian plans.
General goals use the native evaluator, solver/tracker and original pause UI.
Historical manifests without it keep their old rules.
"""
import copy,json
from pathlib import Path
ROOT=Path(__file__).resolve().parent


def capture(patcher):
    from logic.logic import Logic
    from graph.graph_utils import graphAreas
    from rom.addresses import Addresses
    from rom.enemies_objectives_data import enemies_objectives_data
    from utils.objectives import Objectives
    settings=patcher.settings
    if Logic.implementation!='vanilla' or settings['tourian'] not in ('Vanilla','Fast','Disabled'):
        raise ValueError('Unsupported native objective layout or Tourian mode')
    catalog=json.loads((ROOT/'native_objectives.json').read_text())
    byname={g['name']:g for g in catalog['goals']}
    goals=[byname[g.name] for g in Objectives.activeGoals]
    from sm_scavenger import capture as capture_scavenger
    hunt=capture_scavenger(patcher)
    if not goals or any(not g['supported'] and not (g['id']==16 and hunt) for g in goals):
        raise ValueError('Unsupported objective or missing Scavenger order')
    locations=sorted(json.loads((ROOT/'native_locations.json').read_text())['locations'],key=lambda l:l['address'])
    itemlocs={il.Location.Address:il for il in settings['itemLocs'] if not il.Location.isBoss()}
    if set(itemlocs)!={l['address'] for l in locations}:
        raise ValueError('Native objective v1 requires all original item locations')
    itemlocs=[itemlocs[l['address']] for l in locations]
    item_counted=[int(il.Item.Category!='Nothing' and not il.Location.restricted) for il in itemlocs]
    before=copy.deepcopy(patcher.romFile.data)
    # Capture the actual writer decision: it depends on Bomb's item category,
    # the applied tweak and the effective Chozo-robots objective together.
    option_values=copy.deepcopy(patcher.romOptions._values)
    had_accessible=hasattr(patcher,'_accessibleAreasNoBoss')
    previous_accessible=getattr(patcher,'_accessibleAreasNoBoss',None)
    try:
        patcher.romFile.data={}
        patcher.romOptions._values={address:0 for address in option_values}
        patcher.romOptions.write('objectivesSFX',0 if settings['vanillaObjectives'] else 0x80)
        patcher.romOptions.write('objectivesHidden',0x2 if Objectives.hidden else 0)
        patcher._accessibleAreasNoBoss=patcher._getAccessibleAreasNoBoss(settings['itemLocs'])
        patcher.writeSplitLocs(settings['majorsSplit'],settings['itemLocs'],settings['progItemLocs'])
        # This clamps required=0 and excessive requested counts exactly as VARIA.
        patcher.writeObjectives(settings['itemLocs'],settings['tourian'])
        patcher.writeMapTileCount(settings['area'],settings['escapeAttr'] is not None,settings['tourian'])
        data=patcher.romFile.data
        sleep_flag=data[patcher.romOptions._options['BTsleep'].addr]&4
        def word(addr):return data[addr]|data[addr+1]<<8
        area_ids=set()
        for area in graphAreas:
            address=Addresses.getOne('objectives_locs_'+area)
            for offset in range(101):
                value=data[address+offset]
                if value==255:break
                area_ids.add(value)
            else:raise ValueError('Unterminated objective area list')
        names=['Space Pirates','Ki Hunters','Beetoms','Cacatacs','Kagos','Yapping Maws']
        # Use source insertion order, validated against the compiled native order.
        families=list(enemies_objectives_data)
        if families!=names:raise ValueError('Source enemy-family identities changed')
        address=Addresses.getOne('map_area_tiles')
        result=dict(schema=1,catalogSha256=catalog['sha256'],goals=[g['id'] for g in goals],
            names=[g['name'] for g in goals],required=word(Addresses.getOne('objectives_n_objectives_required')),
            flags=int(not settings['vanillaObjectives'])|int(Objectives.hidden)*2|sleep_flag,
            itemMask=word(Addresses.getOne('itemsMask')),beamMask=word(Addresses.getOne('beamsMask')),
            itemCounted=item_counted,areaCounted=[int(il.Location.Id in area_ids) for il in itemlocs],
            enemyTotals=[data[Addresses.getOne(enemies_objectives_data[n]['total_sym'])] for n in families],
            mapTotals=[data[address+i] for i in range(12)],areaLayout=bool(settings['area']))
    finally:
        patcher.romFile.data=before
        patcher.romOptions._values=option_values
        if had_accessible:patcher._accessibleAreasNoBoss=previous_accessible
        elif hasattr(patcher,'_accessibleAreasNoBoss'):del patcher._accessibleAreasNoBoss
    if hunt is not None:
        result.update(schema=2,scavenger=hunt)
    if settings['tourian']=='Fast':result.update(schema=3,tourian='Fast',flags=result['flags']|8)
    from sm_minimizer import capture as capture_minimizer
    mini=capture_minimizer(patcher)
    if mini is not None:result.update(schema=4,minimizer=mini,tourian=settings['tourian'])
    from sm_escape import capture as capture_escape
    escape=capture_escape(patcher)
    if escape is not None:result.update(schema=5,escape=escape,tourian=settings['tourian'],flags=result['flags']|(16 if settings['tourian']=='Disabled' else 0))
    elif settings['tourian']=='Disabled':raise ValueError('Disabled Tourian requires escape')
    validate(result)
    return result


def validate(data):
    from logic.logic import Logic
    from graph.graph_utils import graphAreas
    catalog=json.loads((ROOT/'native_objectives.json').read_text())
    if not isinstance(data,dict) or data.get('schema') not in (1,2,3,4,5) or data.get('catalogSha256')!=catalog['sha256']:
        raise ValueError('Unsupported native objective contract')
    hunt=data.get('scavenger')
    if data['schema']==2 or (data['schema']>=3 and hunt is not None):
        from sm_scavenger import validate as validate_scavenger
        validate_scavenger(hunt)
    elif hunt is not None:raise ValueError('Scavenger requires objective schema 2')
    goals=data.get('goals')
    if not isinstance(goals,list) or not 1<=len(goals)<=18 or any(type(g) is not int or not 0<=g<len(catalog['goals']) or (not catalog['goals'][g]['supported'] and not (g==16 and hunt)) for g in goals) or len(set(goals))!=len(goals):
        raise ValueError('Invalid native goal list')
    if bool(hunt)!=(16 in goals):raise ValueError('Scavenger objective and order disagree')
    if data.get('names')!=[catalog['goals'][g]['name'] for g in goals]:raise ValueError('Native goal identities disagree')
    required=data.get('required');flags=data.get('flags')
    mini=data.get('minimizer')
    escape=data.get('escape');disabled=data.get('tourian')=='Disabled'
    if (data['schema']==5)!=(escape is not None):raise ValueError('Missing escape dependency')
    if data['schema']<5 and (data['schema']==4)!=(mini is not None):raise ValueError('Missing Minimizer dependency')
    if mini is not None and data.get('tourian') not in ('Vanilla','Fast','Disabled'):raise ValueError('Invalid Minimizer Tourian mode')
    if escape is not None:
        from sm_escape import validate as validate_escape
        validate_escape(escape)
        if data.get('tourian') not in ('Vanilla','Fast','Disabled') or disabled!=bool(escape['clock']['flags']&1):raise ValueError('Escape and Tourian disagree')
    if disabled and (escape is None or type(flags) is not int or not flags&16):raise ValueError('Missing Disabled Tourian dependency')
    fast=data['schema']==3 or (data['schema']>=4 and data['tourian']=='Fast')
    if fast and (data.get('tourian')!='Fast' or type(flags) is not int or not flags&8):raise ValueError('Missing Fast Tourian dependency')
    if type(required) is not int or not 1<=required<=len(goals) or type(flags) is not int or flags&~(23 if disabled else 15 if fast else 7):
        raise ValueError('Invalid native objective requirement or flags')
    if flags&4 and 'activate chozo robots' in data['names']:
        raise ValueError('Bomb Torizo cannot sleep with the Chozo robots objective')
    for name,mask in [('itemMask',0xf32f),('beamMask',0x100f)]:
        n=data.get(name)
        if type(n) is not int or n<0 or n&~mask:raise ValueError('Invalid objective equipment mask')
    for name in ['itemCounted','areaCounted']:
        array=data.get(name)
        if not isinstance(array,list) or len(array)!=100 or any(type(v) is not int or v not in (0,1) for v in array):
            raise ValueError('Invalid objective item membership')
    layout=data.get('areaLayout')
    if type(layout) is not bool or Logic.implementation!='vanilla':raise ValueError('Unsupported objective world layout')
    expected=Logic.map_tilecount['area_rando' if layout else 'vanilla_layout']
    maps=[expected[r]-(1 if fast and not layout and i==1 else 0) if i not in (0,11) else 0 for i,r in enumerate(graphAreas)]
    enemies=[62,19,22,20,5,17]
    if mini is not None:
        from sm_minimizer import totals
        maps,enemies=totals(mini)
        if not layout or any((a or b) and not enabled for a,b,enabled in zip(data['itemCounted'],data['areaCounted'],mini['checks'])):raise ValueError('Objective counts include excluded Minimizer checks')
    if escape is not None:
        from sm_escape import map_offsets
        maps=[n-off for n,off in zip(maps,map_offsets(mini))]
    for name,totals in [('mapTotals',maps),('enemyTotals',enemies)]:
        values=data.get(name)
        if not isinstance(values,list) or any(type(n) is not int for n in values) or values!=totals:
            raise ValueError('Native objective v1 cannot interpret a filtered world: '+name)
    return data


def configure_logic(data=None, collected=None):
    """Restore effective goals without re-expanding or re-randomizing them.

    ``collected`` supplies location names, not received item counts. Both the
    solver's visited checks and the native tracker's collection bits therefore
    use the exact source-written membership, including split and empty checks.
    This configures feasibility predicates; it never invents gameplay events.
    """
    from logic.logic import Logic
    from logic.smbool import SMBool
    from rando.Items import ItemManager
    from utils.objectives import Objectives

    disabled=data is not None and data.get('tourian')=='Disabled'
    objectives = Objectives(tourianRequired=not disabled, reset=True)
    Objectives.nbRequiredGoals = 0
    Objectives.permissive = False
    Objectives.hidden = False
    Objectives.totalItemsCount = 100
    Objectives.totalEnemies = None
    if data is None:
        objectives.setVanilla()
        return objectives
    validate(data)
    if collected is None:
        raise ValueError('Native objective logic requires collected locations')
    if data.get('scavenger'):
        names=set(data['scavenger']['locations'])
        Objectives.goals['finish scavenger hunt'].setClearFunc(lambda sm,ap,names=names: SMBool(names.issubset(set(collected()))))
    for name in data['names']:
        objectives.addGoal(name)
    if [g.name for g in Objectives.activeGoals] != data['names']:
        raise ValueError('Effective native objectives conflict with source logic')
    # writeGoals has already resolved "all", including lists longer than nine.
    # setNbRequiredGoals is a request normalizer and would clamp these again.
    Objectives.nbRequiredGoals = data['required']
    Objectives.hidden = bool(data['flags'] & 2)
    Objectives.totalEnemies = dict(zip(
        ('Space Pirates', 'Ki Hunters', 'Beetoms', 'Cacatacs', 'Kagos', 'Yapping Maws'), data['enemyTotals']))
    # Restore the source no-Tourian landing-site requirement between workers.
    Objectives.goals['nothing'].setClearFunc((lambda sm,ap: Objectives.canAccess(sm,ap,'Landing Site')) if disabled else (lambda sm,ap: SMBool(True)))
    ordered = sorted((loc for loc in Logic.locations() if not loc.isBoss()),
                     key=lambda loc: loc.Address)
    if len(ordered) != 100:
        raise ValueError('Native objective logic requires the full location set')
    counted = {loc.Name for loc, flag in zip(ordered, data['itemCounted']) if flag}
    Objectives.totalItemsCount = len(counted)
    for percent in (25, 50, 75, 100):
        threshold = (len(counted) * percent + 99) // 100
        Objectives.goals[f'collect {percent}% items'].setClearFunc(
            lambda sm, ap, threshold=threshold: SMBool(len(counted & set(collected())) >= threshold))

    equipment = [item for item in ItemManager.Items.values() if item.ItemBits or item.BeamBits]
    def upgrades(sm, ap):
        items = beams = 0
        for item in equipment:
            if sm.haveItem(item.Type):
                items |= item.ItemBits
                beams |= item.BeamBits
        return SMBool(items == data['itemMask'] and beams == data['beamMask'])
    Objectives.goals['collect all upgrades'].setClearFunc(upgrades)
    for goal in Objectives.goals.values():
        if goal.gtype != 'items' or goal.area is None:
            continue
        names = {loc.Name for loc, flag in zip(ordered, data['areaCounted'])
                 if flag and loc.GraphArea == goal.area}
        def area_clear(sm, ap, names=names, area=goal.area):
            return sm.wand(SMBool(names.issubset(set(collected()))),
                           Objectives.canReachArea(sm, ap, area))
        goal.setClearFunc(area_clear)
    return objectives


def tracker_progress(data, snapshot, sm, root, maximum):
    """Logical opportunities and observed native progress are distinct evidence.

    VARIA estimates exploration feasibility using accessible locations/APs.
    Only the native evaluator supplies actual visited tiles, kills/interactions
    and latched completion. Older callers without this evidence get nulls.
    """
    from utils.objectives import Objectives
    from utils.parameters import infinity
    if data is None:
        return None
    state = values = None
    if snapshot is not None:
        state = snapshot.get('state')
        ids = snapshot.get('goals')
        values = snapshot.get('values')
        def ints(array, size):
            return isinstance(array, list) and len(array) == size and all(type(n) is int for n in array)
        if (snapshot.get('version') != 1 or not ints(state, 8) or not ints(ids, 18) or
                not isinstance(values, list) or len(values) != 18 or any(not ints(row, 5) for row in values)):
            raise ValueError('Invalid native objective snapshot')
        count = len(data['goals'])
        if (state[0] != count or state[1] != data['required'] or state[6] != data['flags'] or
                ids != data['goals'] + [-1] * (18-count) or any(row != [-1]*5 for row in values[count:])):
            raise ValueError('Native objective snapshot does not match this seed')
        if (any(state[i] not in (0, 1) for i in (4, 5, 7)) or
                any(row[0] not in (0, 1) or row[1] < 0 or row[2] < 0 or
                    not -1 <= row[3] <= 100 or row[4] not in (0, 1) for row in values[:count]) or
                state[2] != sum(row[0] for row in values[:count]) or
                state[3] != max(0, state[1]-state[2])):
            raise ValueError('Inconsistent native objective progress')
    entries = []
    previous = Objectives.maxDiff
    try:
        for index, goal in enumerate(Objectives.activeGoals):
            def possible(limit):
                Objectives.maxDiff = limit
                result = goal.canClearGoal(sm, root)
                return bool(result and result.difficulty <= limit)
            normal, alternative = possible(maximum), possible(infinity)
            progress = values[index] if values is not None else None
            entries.append(dict(index=index, id=data['goals'][index], name=goal.name,
                completable=normal, completableAboveDifficulty=not normal and alternative,
                completed=bool(progress[0]) if progress is not None else None,
                amount=progress[1] if progress is not None else None,
                target=progress[2] if progress is not None else None,
                percent=progress[3] if progress is not None and progress[3] >= 0 else None,
                conditionMet=bool(progress[4]) if progress is not None else None))
    finally:
        Objectives.maxDiff = previous
    return dict(required=data['required'], count=len(entries), goals=entries,
        completedCount=state[2] if state is not None else None,
        requiredMet=bool(state[4]) if state is not None else None,
        allMet=bool(state[5]) if state is not None else None,
        hidden=bool(data['flags'] & 2), revealed=bool(state[7]) if state is not None else not bool(data['flags'] & 2),
        progressSource='native-evaluator' if state is not None else 'unavailable',
        feasibilitySource='VARIA goal predicates; exploration uses source AP/location estimates')
