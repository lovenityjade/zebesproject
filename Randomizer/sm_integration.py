"""Internal VARIA generation service, hosted by libsm_native in-process.

The game calls this module; it is not a launcher. No ROM, SRAM, IPS file or
network service is required. The native new-game menu owns generation and activation.
"""
from __future__ import annotations
import contextlib
import hashlib
import io
import json
from pathlib import Path
import runpy
import sys
import tempfile

ROOT = Path(__file__).resolve().parent
UPSTREAM = ROOT / 'upstream'
REVISION = '72ec1f30b700442d0c30aad6c43801bd64f1e2a5'


def patch_catalog():
    return json.loads((ROOT / 'patches.json').read_text())


class UnverifiedSeed(ValueError):
    pass


def verification_error(verification, rules):
    """Keep the failed route and upstream warnings visible in the game UI."""
    reached = verification['reachableItemCount']
    total = verification['retainedItemCount']
    parts = [f"Seed rejected: {reached}/{total} checks verified at {rules['options']['maxDifficulty']} difficulty."]
    blocked = verification['unavailable']
    if blocked:
        names = '; '.join(blocked[:3])
        if len(blocked) > 3:
            names += f'; and {len(blocked)-3} more'
        parts.append('Blocked checks: ' + names + '.')
    if not verification['completionVerified']:
        parts.append('The selected victory condition has no verified route.')
    elif verification['completionMode'] == 'chozo-relic-hunt':
        parts.append('The tablet goal is reachable, but all checks must also be reachable.')
    notes = rules.get('generationNotes', '').strip()
    if notes:
        parts.append('VARIA: ' + notes)
    parts.append('Review Logic & difficulty and Combat & heat in Randomizer options. No settings were changed.')
    return '\n'.join(parts)


def generate_once(request):
    if not isinstance(request, dict):
        raise ValueError('Request must be an object')
    unknown = set(request) - {'seed', 'skill', 'progression', 'patches', 'options', 'techniques', 'skillSettings', 'noAdvancedTechs', 'relicHunt'}
    if unknown:
        raise ValueError('Unknown options: ' + ', '.join(sorted(unknown)))
    seed = request.get('seed')
    if type(seed) is not int or not 1 <= seed <= 2147483647:
        raise ValueError('seed must be an integer in 1..2147483647')
    skill = request.get('skill', 'casual')
    if skill not in ('newbie', 'casual', 'regular', 'veteran', 'expert', 'master', 'samus', 'solution'):
        raise ValueError('Unsupported skill preset')
    progression = request.get('progression', 'medium')
    selected = request.get('patches', [])
    if not isinstance(selected, list) or any(type(p) is not str for p in selected):
        raise ValueError('patches must be an array of option IDs')
    catalog = {p['id']: p for p in patch_catalog()}
    if len(set(selected)) != len(selected) or any(p not in catalog for p in selected):
        raise ValueError('Unknown or duplicate patch option')
    selected = sorted(selected)
    sys.path.insert(0, str(UPSTREAM))
    # Import only pinned, local logic. DB support is explicitly off even on a
    # developer machine with a mysql installation and configuration.
    import utils.db as db
    db.dbAvailable = False
    from rom.rompatcher import RomPatcher
    from rando.Items import ItemManager
    from sm_options import resolve
    rules = resolve(request)
    requested_settings = {key:request.get(key,{}) for key in ('options','techniques','skillSettings')}
    native_context = {}
    placements = []
    generator_progression = []
    active_logic_patches = []

    def native_placements(patcher):
        from rom.rom_patches import RomPatches
        from utils.objectives import Objectives
        from graph.graph_utils import getAccessPoint
        native_context.update(start=patcher.settings['startLocation'], split=patcher.settings['majorsSplit'],
            tourian=patcher.settings['tourian'], goals=[g.name for g in Objectives.activeGoals],
            goalsRequired=Objectives.nbRequiredGoals,
            spawn=getAccessPoint(patcher.settings['startLocation']).Start['spawn'])
        # Run VARIA's authoritative data writer into its in-memory FakeROM.
        # Consume only the count lists; none of these bytes patch the game ROM.
        from rom.addresses import Addresses
        from graph.graph_utils import graphAreas
        patcher.writeSplitLocs(patcher.settings['majorsSplit'], patcher.settings['itemLocs'], patcher.settings['progItemLocs'])
        hud_ids=set()
        for graph_area in graphAreas:
            address=Addresses.getOne('objectives_locs_'+graph_area)
            for offset in range(101):
                value=patcher.romFile.data[address+offset]
                if value==255:break
                hud_ids.add(value)
            else:raise ValueError('Unterminated VARIA HUD count list')
        from sm_scavenger import capture as capture_scavenger
        hunt=capture_scavenger(patcher)
        if hunt is not None:native_context['scavenger']=hunt
        from sm_native_world import capture
        native_context['world']=capture(patcher,rules)
        from sm_animals import capture as capture_animals
        animals=capture_animals(patcher,rules)
        if animals is not None:native_context['animals']=animals
        from sm_topology import capture as capture_topology
        from sm_minimizer import capture as capture_minimizer,topology as minimizer_topology
        from sm_escape import capture as capture_escape,escape_pairs
        escape=capture_escape(patcher)
        mini=capture_minimizer(patcher)
        native_context['topology']=minimizer_topology(mini) if mini is not None else capture_topology(patcher.settings,escape)
        if escape is not None:native_context['topology'].update(nativeEscape=escape,escapePairs=escape_pairs(escape['routing']))
        from sm_door_colors import capture as capture_door_colors
        doors=capture_door_colors(patcher)
        if doors is not None:native_context['doorColors']=doors
        if not rules['relicHunt']['enabled']:
            from sm_objectives import capture as capture_objectives
            native_context['objectives']=capture_objectives(patcher)
            native_context['goals']=native_context['objectives']['names']
            native_context['goalsRequired']=native_context['objectives']['required']
            # Preserve the requested draw/pool separately; all consumers of
            # rules see the effective list, quota and visibility from VARIA.
            goal_contract=native_context['objectives']
            effective_goals=dict(objective=goal_contract['names'][:],
                nbObjective=len(goal_contract['goals']),nbObjectivesRequired=goal_contract['required'],
                hiddenObjectives='on' if goal_contract['flags']&2 else 'off')
            if rules['options']['objectiveRandom']!='true':effective_goals['distributeObjectives']='off'
            for key,value in effective_goals.items():
                if rules['options'].get(key)!=value:
                    rules['adjustments'].append(f'{key}: {rules["options"].get(key)!r} -> {value!r} (effective VARIA objectives).')
                    rules['options'][key]=value
        active_logic_patches.extend(RomPatches.ActivePatches)
        generator_progression.extend(dict(location=il.Location.Name, item=il.Item.Type)
                                     for il in patcher.settings["progItemLocs"] if not il.Location.isBoss())
        # Replace ROM patching at the library boundary. Generating arbitrary
        # 65816 patch bytes would not implement their behavior in the C core.
        for il in patcher.settings['itemLocs']:
            loc, item = il.Location, il.Item
            if loc.isBoss():
                continue
            kind = 2 if item.Category == 'Nothing' else 0
            code = ItemManager.getItemTypeCode(ItemManager.Items['Reserve'] if kind else item, loc.Visibility)
            if not 0xeed7 <= code <= 0xefcf or (code - 0xeed7) % 4:
                raise ValueError('Non-vanilla item PLM')
            counted = loc.Id in hud_ids
            placements.append(dict(location=loc.Name, address=loc.Address, hudCounted=counted,
                                   visibility=loc.Visibility, item=item.Type, plm=code, kind=kind))
        patcher.romFile.data = {}

    old_patch = RomPatcher.patchRom
    old_argv = sys.argv
    log = io.StringIO()
    try:
        RomPatcher.patchRom = native_placements
        with tempfile.TemporaryDirectory(prefix='sm-seed-') as tmp:
            tmp = Path(tmp)
            preset = rules['options']
            (tmp/'skill.json').write_text(json.dumps(rules['preset']))
            (tmp/'preset.json').write_text(json.dumps(preset))
            sys.argv = [str(UPSTREAM/'randomizer.py'), '--param',
                        str(tmp/'skill.json'),
                        '--randoPreset', str(tmp/'preset.json'), '--seed', str(seed),
                        '--output', str(tmp/'generation.json')]
            with contextlib.redirect_stdout(log), contextlib.redirect_stderr(log):
                try:
                    runpy.run_path(str(UPSTREAM/'randomizer.py'), run_name='__main__')
                except SystemExit as ex:
                    if ex.code not in (0, None):
                        raise ValueError('VARIA generation failed: ' + log.getvalue()[-1800:]) from ex
            output = json.loads((tmp/'generation.json').read_text())
            if output.get('errorMsg'):
                if len(placements)!=100:raise ValueError(output['errorMsg'])
                rules['generationNotes']=output['errorMsg']
    finally:
        RomPatcher.patchRom = old_patch
        sys.argv = old_argv
    placements.sort(key=lambda p: p['address'])
    if len(placements) != 100 or len({p['address'] for p in placements}) != 100:
        raise ValueError('Expected exactly 100 unique native item locations')
    from sm_validation import verify_progression
    from sm_relics import place_relics
    place_relics(placements,rules['relicHunt'],seed,generator_progression)
    for placement in placements:
        if placement['kind']==1:placement['hudCounted']=True
        elif placement['kind']==2:placement['hudCounted']=False
    verification = verify_progression(placements, skill, active_logic_patches, rules, native_context)
    if not verification['allItemsReachable'] or not verification['completionVerified']:
        raise UnverifiedSeed(verification_error(verification, rules))
    manifest = dict(schema=1, seed=seed, skill=skill, progression=progression,
                    upstream=REVISION, placements=placements, patches=selected,
                    rules=rules, requestedSettings=requested_settings, nativeContext=native_context,
                    activation='staged-only', generatorProgression=generator_progression,
                    logicPatches=sorted(set(active_logic_patches)),
                    solverVerification=verification,
                    requiredNativeBehavior=['native-suits-v1', 'native-initial-doors-v1', 'native-start-v1', 'hud-counts-v1', 'seed-interface-v1', 'zebes-awake', 'morph-eye-item-check',
                                            'item-location-save-identity',
                                            'blue-brinstar-blue-door', 'red-tower-blue-doors'],
                    pendingPatches=[p for p in selected if catalog[p]['status'] != 'native'])
    if native_context['tourian']=='Fast':manifest['requiredNativeBehavior'].append('native-fast-tourian-v1')
    if 'nativeEscape' in native_context['topology']:manifest['requiredNativeBehavior'].append('native-escape-v1')
    if 'animals' in native_context:manifest['requiredNativeBehavior'].append('native-animals-v1')
    if 'scavenger' in native_context:manifest['requiredNativeBehavior'].append('native-scavenger-v1')
    if 'objectives' in native_context:manifest['requiredNativeBehavior'].append('native-objectives-v1')
    if 'doorColors' in native_context:manifest['requiredNativeBehavior'].append('native-door-colors-v1')
    if rules['options'].get('hideItems')=='on':manifest['requiredNativeBehavior'].append('hidden-items-v1')
    if 'indicators' in native_context['world']:manifest['requiredNativeBehavior'].append('native-door-indicators-v1')
    if 'native' in native_context['topology']:manifest['requiredNativeBehavior'].append('native-boss-connections-v1')
    if 'nativeMinimizer' in native_context['topology']:manifest['requiredNativeBehavior'].append('native-minimizer-v1')
    if 'nativeAreas' in native_context['topology']:manifest['requiredNativeBehavior'].append('native-area-connections-v1')
    for key,behavior in [('itemsounds','item-sounds-v1'),('spinjumprestart','respin-v1'),('Infinite_Space_Jump','infinite-spacejump-v1'),('relaxed_round_robin_cf','round-robin-cf-v1'),('rando_speed','momentum-landing-v1'),('nerfedCharge','nerfed-charge-v1')]:
        if rules['options'].get(key)=='on':manifest['requiredNativeBehavior'].append(behavior)
    if rules['options'].get('elevators_speed')=='on':manifest['requiredNativeBehavior'].append('fast-elevators-v1')
    if rules['options'].get('fast_doors')=='on':manifest['requiredNativeBehavior'].append('fast-doors-v1')
    if rules['options'].get('energyQty')=='ultra sparse':manifest['requiredNativeBehavior'].append('nerfed-rainbow-v1')
    if rules['options'].get('refill_before_save')=='on':manifest['requiredNativeBehavior'].append('save-refill-v1')
    if any(p['kind']==1 for p in placements):manifest['requiredNativeBehavior'].append('chozo-relic-v1')
    if any(p['kind']==2 for p in placements):manifest['requiredNativeBehavior'].append('empty-pickup-v1')
    if rules['relicHunt']['enabled']:
        manifest['generatorProgressionBeforeRelicReplacement']=manifest.pop('generatorProgression')
        manifest['generatorProgression']=[dict(location=p['location'],item=p['item']) for p in verification['progressionLog'] if p['location'] in {q['location'] for q in placements}]
    canonical = json.dumps({k:manifest[k] for k in ('schema','seed','upstream','placements','patches','logicPatches','requiredNativeBehavior','rules','nativeContext','requestedSettings')}, sort_keys=True, separators=(',', ':')).encode()
    manifest['sha256'] = hashlib.sha256(canonical).hexdigest()
    return manifest


def candidate_seed(seed, attempt):
    if not attempt:
        return seed
    digest = hashlib.sha256(f"SM-native-seed-v1:{seed}:{attempt}".encode()).digest()
    return (int.from_bytes(digest[:4], 'little') & 0x7fffffff) or 1


def generate_json(request_json, attempt=0):
    # libsm_native runs each attempt in a NEW subinterpreter. VARIA has global
    # class registries as well as module state; a retry must reset both.
    try:
        request = json.loads(request_json)
        from sm_preflight import check_settings
        compatibility = check_settings(request)
        if not compatibility['ok']:
            return json.dumps(dict(ok=False, retry=False,
                error='\n'.join(i['message'] for i in compatibility['issues']),
                settingsIssues=compatibility['issues'], generationAttempts=0))
        candidate = dict(request) if isinstance(request, dict) else request
        if attempt:
            candidate['seed'] = candidate_seed(request['seed'], attempt)
        manifest = generate_once(candidate)
        manifest['upstreamSeed'] = candidate['seed']
        manifest['seed'] = request['seed']
        manifest['generationAttempts'] = attempt + 1
        manifest['rejectedCandidates'] = [dict(upstreamSeed=candidate_seed(request['seed'], i),
            reason='No verified complete route within requested difficulty') for i in range(attempt)]
        del manifest['sha256']
        canonical = json.dumps({k:manifest[k] for k in ('schema','seed','upstream','placements','patches','logicPatches','requiredNativeBehavior','rules','nativeContext','requestedSettings')}, sort_keys=True, separators=(',', ':')).encode()
        manifest['sha256'] = hashlib.sha256(canonical).hexdigest()
        from sm_tracker_data import build_tracker_data
        manifest['tracker'] = build_tracker_data(manifest)
        manifest['trackerSha256'] = hashlib.sha256(json.dumps(manifest['tracker'], sort_keys=True, separators=(',', ':')).encode()).hexdigest()
        result = dict(ok=True, manifest=manifest)
    except UnverifiedSeed as ex:
        result = dict(ok=False, retry=attempt < 7, error=str(ex), generationAttempts=attempt+1)
    except Exception as ex:
        result = dict(ok=False, error=f'{type(ex).__name__}: {ex}')
    return json.dumps(result, sort_keys=True, separators=(',', ':'))
