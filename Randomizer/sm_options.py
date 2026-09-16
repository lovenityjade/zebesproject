"""Explicit native seed rules shared by generation, validation and tracking.

No presentation preferences or community credentials are included. Historical
four-field requests retain their exact effective defaults.
"""
import copy
import json
import random
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parent
SKILLS = ('newbie', 'casual', 'regular', 'veteran', 'expert', 'master', 'samus', 'solution')
# Ordinary inputs / the intended full-length shinespark. No timing shortcuts,
# clipping, damage boosts, suitless traversal or infinite bomb jumps.
BASIC_TECHNIQUES = frozenset(('WallJump', 'ShineSpark', 'MidAirMorph', 'CrouchJump', 'UnequipItem'))
ENUMS = {
    'majorsSplit': ('Full', 'FullWithHUD', 'Major', 'Chozo', 'Scavenger'),
    'progressionSpeed': ('slowest', 'slow', 'medium', 'fast', 'fastest', 'basic', 'VARIAble', 'speedrun'),
    'progressionDifficulty': ('easier', 'normal', 'harder'),
    'morphPlacement': ('early', 'late', 'normal'),
    'maxDifficulty': ('easy', 'medium', 'hard', 'harder', 'hardcore', 'mania', 'infinity'),
    'energyQty': ('ultra sparse', 'sparse', 'medium', 'vanilla'),
    'gravityBehaviour': ('Vanilla', 'Balanced', 'Progressive'),
    'areaRandomization': ('off', 'full', 'light'),
    'tourian': ('Vanilla', 'Fast', 'Disabled'),
    'logic': ('vanilla', 'mirror'),
}
TOGGLES = ('suitsRestriction', 'hideItems', 'strictMinors', 'funCombat', 'funMovement',
    'funSuits', 'bossRandomization', 'doorsColorsRando', 'allowGreyDoors', 'escapeRando',
    'removeEscapeEnemies', 'minimizer', 'areaLayout', 'layoutPatches', 'variaTweaks',
    'nerfedCharge', 'relaxed_round_robin_cf', 'revealMap', 'hud', 'itemsounds',
    'elevators_speed', 'fast_doors', 'spinjumprestart', 'rando_speed',
    'Infinite_Space_Jump', 'refill_before_save', 'better_reserves', 'animals',
    'scavRandomized', 'hiddenObjectives', 'distributeObjectives', 'raceMode')
NUMBERS = {'missileQty': (1,9), 'superQty': (1,9), 'powerBombQty': (1,9),
    'minorQty': (7,100), 'scavNumLocs': (4,17), 'minimizerQty': (30,100),
    'nbObjective': (1,18), 'nbObjectivesRequired': (1,18)}


def resolve(request):
    from utils.parameters import Knows, isKnows, Settings
    from utils.objectives import Objectives
    from graph.graph_utils import GraphUtils
    from utils.utils import getDefaultMultiValues, getRandomizerDefaultParameters
    skill = request.get('skill', 'casual')
    if skill not in SKILLS:
        raise ValueError('Unsupported skill preset')
    preset = json.loads((ROOT/'upstream/standard_presets'/f'{skill}.json').read_text())
    preset = {key: copy.deepcopy(preset.get(key, {})) for key in ('Knows','Settings','Controller')}
    techniques = request.get('techniques', {})
    if not isinstance(techniques, dict):
        raise ValueError('techniques must be an object')
    for name, value in techniques.items():
        if not isKnows(name) or not hasattr(Knows,name) or not isinstance(value,list) or len(value)!=2 or type(value[0]) is not bool or type(value[1]) not in (int,float) or not 0<=value[1]<=800:
            raise ValueError('Invalid technique: '+str(name))
        preset['Knows'][name]=value[:]
    adjustments=request.get('skillSettings',{})
    if not isinstance(adjustments,dict):raise ValueError('skillSettings must be an object')
    for name,value in adjustments.items():
        table=next((table[name] for table in (Settings.bossesDifficultyPresets,Settings.hellRunPresets,Settings.hardRoomsPresets) if name in table),None)
        if table is None or value not in table:raise ValueError('Invalid skill setting: '+str(name))
        preset['Settings'][name]=value
    no_advanced=request.get('noAdvancedTechs',False)
    if type(no_advanced) is not bool:raise ValueError('noAdvancedTechs must be boolean')
    if no_advanced:
        # Enumerate ALL known techniques, including those omitted by old JSON
        # presets. Otherwise enabled upstream defaults leak into this mode.
        for name in vars(Knows):
            if isKnows(name):
                value=getattr(Knows,name)
                preset['Knows'][name]=[name in BASIC_TECHNIQUES,1 if name in BASIC_TECHNIQUES else 0]
        preset['Settings']['Ice']='No thanks'
        preset['Settings']['MainUpperNorfair']='No thanks'
        preset['Settings']['LowerNorfair']='Default'
    options=getRandomizerDefaultParameters()
    options.update(json.loads((ROOT/'upstream/rando_presets/vanilla.json').read_text()))
    for key in TOGGLES:options.setdefault(key,'off')
    options['objectiveRandom']='false'
    options.update(startLocation='Landing Site',progressionSpeed=request.get('progression','medium'),maxDifficulty='medium',morphPlacement='early',tourian='Vanilla',objective=['kill all G4'],hud='on',revealMap='on',better_reserves='on')
    for key in NUMBERS:
        if key in options and options[key] not in ('off','random'):
            options[key]=float(options[key]) if key in ('missileQty','superQty','powerBombQty') else int(options[key])
    baseline=copy.deepcopy(options)
    selected=request.get('options',{})
    if not isinstance(selected,dict):raise ValueError('options must be an object')
    if selected.get('logic','vanilla')!='vanilla' or selected.get('raceMode','off')!='off':
        raise ValueError('Mirror and Race are outside the supported scope of this port.')
    multiple=getDefaultMultiValues()
    starts=GraphUtils.getStartAccessPointNames()
    goals=Objectives.getAllGoals(exclude=True)
    manual_goals=Objectives.getAllGoals(exclude=False)
    for key,value in selected.items():
        if key in ENUMS:
            if value not in ENUMS[key] and value!='random':raise ValueError('Invalid '+key)
        elif key in TOGGLES:
            if value not in ('on','off','random'):raise ValueError('Invalid '+key)
        elif key in NUMBERS:
            if key=='nbObjectivesRequired' and value=='off':pass
            elif value=='random':pass
            elif type(value) not in (int,float) or not NUMBERS[key][0]<=value<=NUMBERS[key][1] or (key not in ('missileQty','superQty','powerBombQty') and int(value)!=value):raise ValueError('Invalid '+key)
        elif key=='startLocation':
            if value not in starts and value!='random':raise ValueError('Invalid startLocation')
        elif key=='objectiveRandom':
            if value not in ('true','false'):raise ValueError('Invalid objectiveRandom')
        elif key in ('objective','objectiveMultiSelect'):
            allowed=manual_goals if key=='objective' else goals
            if not isinstance(value,list) or not value or len(value)>18 and key=='objective' or any(v not in allowed for v in value) or len(set(value))!=len(value):raise ValueError('Invalid '+key)
        elif key.endswith('MultiSelect') and key[:-11] in multiple:
            allowed=starts if key=='startLocationMultiSelect' else multiple[key[:-11]]
            if not isinstance(value,list) or not value or any(v not in allowed for v in value) or len(value)!=len(set(value)):raise ValueError('Invalid '+key)
        elif key in ('layoutCustom','areaLayoutCustom','variaTweaksCustom'):
            from rom.rom_patches import groups
            if not isinstance(value,list) or any(type(v) is not str or v not in groups[key[:-6]] for v in value) or len(value)!=len(set(value)):raise ValueError('Invalid '+key)
        else:raise ValueError('Unknown gameplay option: '+key)
        options[key]=copy.deepcopy(value)
    if options['progressionSpeed'] not in ENUMS['progressionSpeed'] and options['progressionSpeed']!='random':raise ValueError('Unsupported progression')
    # Resolve random controls once, independently of VARIA's mutable RNG.
    # Requested allowed sets stay in requestedSettings; rules carry the values
    # actually used by generation, verification and the live tracker.
    rng=random.Random(hashlib.sha256(('SM-options-v1:'+str(request.get('seed',1))).encode()).digest())
    for key in sorted(options):
        if options[key]!='random':continue
        # VARIA intersects starts with skill/Morph/area and draws objective
        # counts from the effective eligible goals. Keep its own distributions
        # and capture the final start/list/quota in the native contract.
        if key in ('startLocation','nbObjective','nbObjectivesRequired','scavNumLocs'):continue
        if key in ENUMS or key=='startLocation':
            choices=options.get(key+'MultiSelect',starts if key=='startLocation' else ENUMS[key])
            if key=='maxDifficulty':choices=[v for v in choices if v!='infinity']
            options[key]=rng.choice(choices)
        elif key in TOGGLES:options[key]=rng.choice(('off','on'))
        elif key in NUMBERS:
            lo,hi=NUMBERS[key]
            options[key]=rng.randint(lo*10,hi*10)/10 if key in ('missileQty','superQty','powerBombQty') else rng.randint(30,60) if key=='minimizerQty' else rng.randint(lo,hi)
    adjustments_log=[]
    if options['tourian']=='Disabled':
        for key,value in dict(escapeRando='on',removeEscapeEnemies='off').items():
            if options[key]!=value:
                adjustments_log.append(f'{key}: {options[key]!r} -> {value!r} (Disabled Tourian dependency).')
                options[key]=value
    if options['minimizer']=='on':
        forced=dict(areaRandomization='full',bossRandomization='on',suitsRestriction='off')
        if options['minimizerQty']<100 and options['majorsSplit'] not in ('Full','FullWithHUD'):forced['majorsSplit']='Full'
        for key,value in forced.items():
            if options[key]!=value:
                adjustments_log.append(f'{key}: {options[key]!r} -> {value!r} (Minimizer dependency).')
                options[key]=value

    if options['progressionSpeed'] in ('speedrun','basic') and options['progressionDifficulty']!='normal':
        options['progressionDifficulty']='normal'
        adjustments_log.append('Placement difficulty set to normal for speedrun/basic progression, as in VARIA.')
    pool_keys={'animals','itemsounds','spinjumprestart','Infinite_Space_Jump','gravityBehaviour','escapeRando','removeEscapeEnemies','minimizer','minimizerQty','tourian','scavNumLocs','scavRandomized','objective','objectiveRandom','nbObjective','nbObjectivesRequired','hiddenObjectives','distributeObjectives',
        'areaRandomization','doorsColorsRando','allowGreyDoors','bossRandomization','startLocation','layoutPatches','layoutCustom','areaLayout','areaLayoutCustom','variaTweaks','variaTweaksCustom','majorsSplit','progressionSpeed','progressionDifficulty','morphPlacement','maxDifficulty',
        'energyQty','suitsRestriction','strictMinors','missileQty','superQty','powerBombQty','minorQty',
        'nerfedCharge','relaxed_round_robin_cf','rando_speed','funCombat','funMovement','funSuits','refill_before_save','elevators_speed','fast_doors','hideItems','hud','revealMap','better_reserves'}
    missing=[]
    for key,value in selected.items():
        if key.endswith('MultiSelect'):continue # candidate pools do not alter native behavior
        if key not in pool_keys and options[key]!=baseline.get(key):missing.append(key)
    if missing:raise ValueError('Native behavior not implemented yet: '+', '.join(sorted(set(missing))))
    relic=request.get('relicHunt',{})
    if not isinstance(relic,dict) or set(relic)-{'enabled','placed','required','escapeMinutes'}:raise ValueError('Invalid relicHunt')
    relic=dict(enabled=relic.get('enabled',False),placed=relic.get('placed',30),required=relic.get('required',20),escapeMinutes=relic.get('escapeMinutes',5))
    if type(relic['enabled']) is not bool or any(type(relic[k]) is not int for k in ('placed','required')) or not 1<=relic['required']<=relic['placed']<=60:raise ValueError('Relic quota must satisfy 1 <= required <= placed <= 60')
    if type(relic['escapeMinutes']) is not int or relic['escapeMinutes'] not in (3,5,6,7,10):raise ValueError('Tablet escape time must be 3, 5, 6, 7 or 10 minutes')
    if relic['enabled'] and options['tourian']!='Vanilla':raise ValueError('Chozo Relic Hunt already bypasses Tourian')
    if relic['enabled'] and options['escapeRando']=='on':raise ValueError('Chozo Relic Hunt uses its own quota escape; VARIA escape route randomization cannot be combined')
    if relic['enabled'] and options['majorsSplit']=='Scavenger':raise ValueError('Chozo Relic Hunt and Scavenger are separate completion modes')
    return dict(options=options,preset=preset,noAdvancedTechs=no_advanced,relicHunt=relic,adjustments=adjustments_log)
