"""Versioned native start and ordered ROM-data plan from effective VARIA settings."""
import json
from pathlib import Path
ROOT=Path(__file__).resolve().parent

def capture(patcher,rules):
    from graph.graph_utils import getAccessPoint
    from rom.rom_patches import getPatchSet,getPatchSetsFromPatcherSettings,groups
    settings=patcher.settings
    options=rules['options']
    effective={'startLocation':settings['startLocation'],'majorsSplit':settings['majorsSplit'],
        'morphPlacement':settings['randoSettings'].restrictions['Morph'],
        'suitsRestriction':'on' if settings['randoSettings'].restrictions['Suits'] else 'off',
        'hud':'on' if settings['hud'] else 'off',
        'revealMap':'on' if settings['revealMap'] else 'off'}
    if settings['escapeAttr'] is not None:
        effective.update(tourian=settings['tourian'],escapeRando='on',removeEscapeEnemies='on' if settings['escapeRandoRemoveEnemies'] else 'off')
    if settings['majorsSplit']=='Scavenger':
        effective.update(progressionSpeed=settings['randoSettings'].progSpeed,
            scavNumLocs=len(settings['progItemLocs']))
    for group,key in [('layout','layoutPatches'),('areaLayout','areaLayout'),('variaTweaks','variaTweaks')]:
        effective[key]='on' if settings[group] else 'off'
        effective[group+'Custom']=settings.get(group+'Custom') or []
    for key,value in effective.items():
        if options.get(key)!=value:
            rules['adjustments'].append(f'{key}: {options.get(key)!r} -> {value!r} (effective VARIA setting).')
            options[key]=value
    catalog=json.loads((ROOT/'native_world_data.json').read_text())
    available={p['name']:p['id'] for p in catalog['patches']}
    selected=[]
    for name in getPatchSetsFromPatcherSettings(settings):
        if name in ('area','boss','doorsColorsRando','fast_tourian','minimizer_bosses','areaEscape') or any(name in members for members in groups.values()):
            selected.extend(getPatchSet(name,'vanilla').get('ips',[]))
    start=getAccessPoint(settings['startLocation']).Start
    if start.get('save'):selected.append(start['save'])
    selected.extend(start.get('rom_patches',[]))
    selected=list(dict.fromkeys(selected))
    # Watering Hole door ASM has an exact native C counterpart in sm_start_door.
    code=[p for p in selected if p in ('rando_escape_common.ips','rando_escape.ips','map_data_escape_rando.ips','rando_escape_ws_fix.ips','minimizer_bosses.ips','minimizer_tourian_common.ips','minimizer_tourian.ips','open_zebetites.ips','wh_open_tube.ips','door_transition.ips','beam_doors_plms.ips','beam_doors_gfx.ips','red_doors.ips') or any(x['name']==p and x.get('nativeBehavior') for x in catalog['patches'])]
    area_catalog=json.loads((ROOT/'native_areas.json').read_text()) if settings['area'] else None
    area_names={p['name'] for p in area_catalog['patches']} if area_catalog else set()
    module_patches=[p for p in selected if p in area_names and p not in available]
    missing=[p for p in selected if p not in available and p not in code and p not in module_patches]
    if missing:raise ValueError('Native patch behavior not implemented yet: '+', '.join(missing))
    world=dict(schema=1,catalogSha256=catalog['sha256'],startSpawn=start['spawn'],
                startDoors=start.get('doors',[]),dataPatches=[p for p in selected if p in available],
                dataPatchIds=[available[p] for p in selected if p in available],nativeCode=code)
    if area_catalog:world['areaPatches']=module_patches
    from sm_initial_doors import capture as capture_initial_doors
    world['initialDoors']=capture_initial_doors(patcher)
    if 'door_indicators_plms.ips' in selected:
        from utils.doorsmanager import DoorsManager,IndicatorFlag
        flags=IndicatorFlag.Standard | (IndicatorFlag.AreaRando if settings['area'] else 0) | (IndicatorFlag.DoorRando if settings['doorsColorsRando'] else 0)
        definitions=next(p['indicatorLocations'] for p in catalog['patches'] if p['name']=='door_indicators_plms.ips')
        names={p['name']:i for i,p in enumerate(definitions)}
        world['indicators']=[]
        for name,plm in DoorsManager.getIndicatorPLMs(flags).items():
            if name not in names or not ((0xfbb0<=plm<=0xfc0a and (plm-0xfbb0)%6==0) or (0xf60b<=plm<=0xf665 and (plm-0xf60b)%6==0)):
                raise ValueError('Native door indicator not implemented: '+name)
            world['indicators'].append(dict(locationId=names[name],plm=plm))
    return world
