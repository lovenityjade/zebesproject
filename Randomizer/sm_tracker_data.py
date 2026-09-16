"""Versioned data for the future native trackers; no tracker UI or spoiler reveal."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parent

def build_tracker_data(manifest):
    from logic.logic import Logic
    from utils.parameters import Knows, Settings, isKnows
    from utils.doorsmanager import DoorsManager
    from graph.graph_utils import vanillaTransitions, vanillaBossesTransitions, vanillaEscapeTransitions
    from rando.Items import ItemManager
    rules=manifest.get('rules',{})
    context=manifest.get('nativeContext',{})
    from sm_topology import from_context
    topology=from_context(context)
    from sm_door_colors import apply_context
    apply_context(context)
    if 'doorColors' in context:topology['doorColors']={k:v for k,v in context['doorColors'].items() if k!='doors'}
    geometry = json.loads((ROOT/'native_locations.json').read_text())
    by_address = {entry['address']: entry for entry in geometry['locations']}
    logic_locations = {loc.Name: loc for loc in Logic.locations() if not loc.isBoss()}
    locations = []
    for index, placement in enumerate(manifest['placements']):
        native = by_address[placement['address']]
        if native['name'] != placement['location']:
            raise ValueError('Tracker/native location identity mismatch')
        loc = logic_locations[placement['location']]
        locations.append(dict(native, originalVisibility=native['visibility'], visibility=placement['visibility'], index=index, item=placement['item'], plm=placement['plm'], kind=placement.get('kind',0),
                             graphArea=loc.GraphArea, solveArea=loc.SolveArea,
                             accessPoints=sorted(loc.AccessFrom),
                             logicReference='graph/vanilla/graph_locations.py:'+loc.Name))
    knows = {name: dict(enabled=getattr(Knows,name).bool, difficulty=getattr(Knows,name).difficulty)
             for name in vars(Knows) if isKnows(name)}
    source_paths = ['logic', 'graph/vanilla', 'utils/parameters.py', 'utils/doorsmanager.py',
                    'utils/objectives.py', 'graph/graph.py', 'graph/graph_utils.py', 'rom/rom_patches.py']
    files = []
    for part in source_paths:
        path = ROOT/'upstream'/part
        files.extend(sorted(path.rglob('*.py')) if path.is_dir() else [path])
    source_hashes = {str(p.relative_to(ROOT/'upstream')): hashlib.sha256(p.read_bytes()).hexdigest() for p in files}
    result = dict(schema=1, upstream=manifest['upstream'], seed=manifest['seed'],
        seedFingerprint=manifest['sha256'], nativeRomSha1=geometry['romSha1'],
        settings=dict(skill=manifest['skill'], progression=rules.get('options',{}).get('progressionSpeed',manifest['progression']),
            start=context.get('start','Landing Site'), startingInventory={}, split=context.get('split','Full'), morphPlacement=rules.get('options',{}).get('morphPlacement','early'),
            noAdvancedTechs=rules.get('noAdvancedTechs',False), relicHunt=rules.get('relicHunt',dict(enabled=False,placed=30,required=20)),
            objectives=['collect Chozo relics','return to ship'] if rules.get('relicHunt',{}).get('enabled') else context.get('goals',['kill all G4']),objectivesRequired=2 if rules.get('relicHunt',{}).get('enabled') else context.get('goalsRequired',1),tourian=context.get('tourian','Vanilla'),
            maximumDifficulty=manifest['solverVerification']['maximumDifficulty'],
            goals=['ChozoRelicQuota','returnToShip'] if rules.get('relicHunt',{}).get('enabled') else [*context['goals'],'escapeToShip'] if context.get('tourian')=='Disabled' else ['Kraid','Phantoon','Draygon','Ridley','MotherBrain','escapeToShip'],
            logicPatches=manifest['logicPatches'], nativeBehavior=manifest['requiredNativeBehavior'],
            knows=knows, hardRooms=Settings.hardRooms, hellRuns=Settings.hellRuns,
            bossesDifficulty=Settings.bossesDifficulty,
            preset=rules.get('preset') or {k:v for k,v in json.loads((ROOT/'upstream/standard_presets'/f"{manifest['skill']}.json").read_text()).items() if k in ('Knows','Settings','Controller')}),
        locations=locations,
        items=[dict(type=i.Type, category=i.Category, code=i.Code, itemBits=i.ItemBits, beamBits=i.BeamBits) for i in ItemManager.Items.values()],
        bosses=[dict(name=n,area=a,mask=m) for n,a,m in [
            ('Kraid',1,1),('Phantoon',3,1),('Draygon',4,1),('Ridley',2,1),('MotherBrain',5,2)]],
        topology=dict(**topology,
            accessPoints=[dict(name=ap.Name, graphArea=ap.GraphArea, internal=ap.Internal,
                boss=ap.Boss, escape=ap.Escape, room=ap.RoomInfo, entry=ap.EntryInfo, exit=ap.ExitInfo,
                connections=sorted(ap.intraTransitions)) for ap in Logic.accessPoints()],
            doors={name:dict(color=d.color, facing=int(d.facing), address=d.address, openedBit=d.id,
                hidden=False) for name,d in DoorsManager.doors.items()}),
        logicSources=source_hashes,
        runtime=dict(snapshotAbi='sm_tracker_snapshot', snapshotVersion=1,
            units=dict(health='energy', ammoCapacity='rounds', ammoPackSize=5,
                       energyTankCapacity=100, baseHealth=99, reserveTankCapacity=100),
            collection='native PLM location bit, independent of received item',
            persistenceKey=['profileId','seedFingerprint','slot'],
            worldValidity='snapshot.world_valid; do not publish inventory while selecting a save'),
        visibility=dict(placements='spoiler data; never reveal implicitly',
            discoveredMap='native SRAM exploration', observedItems='unknown until observed',
            annotations='separate per-profile and per-slot data'))
    # Force plain JSON now, so incompatible upstream objects fail generation.
    if 'objectives' in context:
        result['settings']['nativeObjectives']=context['objectives']
        result['settings']['goals']=list(context['objectives']['names'])+['MotherBrain','escapeToShip']
    if 'scavenger' in context:result['settings']['scavenger']=context['scavenger']
    initial=context.get('world',{}).get('initialDoors')
    if initial:result['settings']['initialDoors']=initial['opened']
    return json.loads(json.dumps(result, sort_keys=True))
