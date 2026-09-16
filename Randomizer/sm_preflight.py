"""Read-only settings checks. No seed placement, ROM, save or network access.

Passing preflight is not a proof of a playable seed. Generation still replays
all checks and the selected victory condition through the independent solver.
"""
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent


def check_settings(request):
    issues = []
    def issue(message, page=1, other=None):
        issues.append(dict(message=message, page=page, otherPage=other))
    if not isinstance(request, dict):
        return dict(ok=False, issues=[dict(message='Settings must be an object.', page=0, otherPage=None)], notes=[])
    if type(request.get('seed', 0)) is not int or not 0 <= request.get('seed', 0) <= 2147483647:
        issue('Seed number: leave it blank or enter a number from 1 to 2147483647.', 0)
    selected = request.get('options', {})
    relic = request.get('relicHunt', {})
    if isinstance(selected, dict) and isinstance(relic, dict) and relic.get('enabled') is True:
        # Report independent conflicts together, with links to both controls.
        for key, default, allowed, label, page in (
                ('tourian', 'Vanilla', ('Vanilla',), 'Tourian must be Vanilla', 4),
                ('escapeRando', 'off', ('off',), 'Escape randomization must be off', 4),
                ('majorsSplit', 'Full', ('Full','FullWithHUD','Major','Chozo'), 'Item split cannot be Scavenger', 3)):
            value = selected.get(key, default)
            if value == 'random':
                choices = selected.get(key+'MultiSelect', [])
                compatible = bool(choices) and all(v in allowed for v in choices)
            else:
                compatible = value in allowed
            if not compatible:
                issue(f'Chozo Relic Hunt: {label}. Change that setting or disable the tablet hunt.', 5, page)
        if issues:
            return dict(ok=False, issues=issues, notes=[])
    sys.path.insert(0, str(ROOT/'upstream'))
    from sm_options import resolve
    try:
        rules = resolve(request)
    except (ValueError, KeyError, TypeError, IndexError) as ex:
        message = str(ex)
        page = 5 if any(k in message for k in ('Relic', 'relic', 'quota', 'Scavenger', 'objective')) else 8 if 'skill setting' in message else 7 if 'technique' in message else 1
        issue(message, page)
        return dict(ok=False, issues=issues, notes=[])
    from sm_integration import patch_catalog
    patches = request.get('patches', [])
    if not isinstance(patches, list) or any(type(p) is not str for p in patches) or len(set(patches)) != len(patches) or set(patches)-{p['id'] for p in patch_catalog()}:
        issue('Gameplay patches: unknown or duplicate patch selection.', 6)
    options = rules['options']
    notes = list(rules['adjustments'])
    if options['objectiveRandom'] == 'false' and not rules['relicHunt']['enabled']:
        from utils.objectives import Objectives
        # Apply the same source exclusions as VARIA instead of silently dropping
        # one of the player's manually selected objectives.
        goals = Objectives(tourianRequired=options['tourian'] != 'Disabled', reset=True)
        Objectives.startAP = options['startLocation'] if options['startLocation'] != 'random' else 'Landing Site'
        names = options['objective']
        if len(names) > Objectives.maxActiveGoals:
            issue(f'Selected objectives: at most {Objectives.maxActiveGoals} can be active.', 5)
        else:
            for name in names:
                if goals.conflict(Objectives.goals[name]):
                    active = ', '.join(g.name for g in Objectives.activeGoals)
                    issue(f'Objective "{name}" conflicts with the current selection ({active}) or the selected start / Tourian mode. Remove the conflicting objective or adjust World & escape.', 5, 4)
                else:
                    goals.addGoal(name)
        # Generation runs later in this same isolated interpreter. Its objective
        # builder expects a fresh list; validation must not pre-populate it.
        Objectives.resetGoals()
    # Resolve uses seed-dependent draws for random settings. A draft without
    # its final number must not be certified for just one possible draw.
    random_keys = [k for k,v in request.get('options', {}).items() if v == 'random']
    if random_keys:
        notes.append('Random choices are checked again after they are drawn during generation.')
    # A deliberately optimistic inventory gives a lower bound, not an estimate
    # of the actual seed: if even this fails, no placement can fix these rules.
    # Skip optional bosses in minimizer worlds and unknown randomized limits.
    if options['minimizer'] == 'off' and 'minimizer' not in random_keys and 'maxDifficulty' not in random_keys:
        from logic.logic import Logic
        from rom.flavor import RomFlavor
        from rom.rom_patches import RomPatches
        from utils.utils import PresetLoader
        from utils.parameters import text2diff, getDiffThreshold
        from logic.helpers import diffValue2txt
        Logic.factory('vanilla', new=True)
        RomFlavor.factory(str(ROOT/'upstream'))
        RomPatches.ActivePatches = []
        PresetLoader.factory(rules['preset']).load()
        from logic.smboolmanager import SMBoolManager
        sm = SMBoolManager()
        for name in sm.percentItems:
            sm.addItem(name)
        for name, count in [('ETank',14),('Reserve',4),('Missile',66),('Super',66),('PowerBomb',66)]:
            for _ in range(count):sm.addItem(name)
        limit = getDiffThreshold(text2diff[options['maxDifficulty']])
        bosses = [('Kraid','enoughStuffsKraid'),('Phantoon','enoughStuffsPhantoon'),('Draygon','enoughStuffsDraygon'),('Ridley','enoughStuffsRidley')]
        if not rules['relicHunt']['enabled'] and options['tourian'] != 'Disabled':
            bosses.append(('MotherBrain','enoughStuffsMotherbrain'))
        for name, method in bosses:
            result = getattr(sm, method)()
            if not result.bool or result.difficulty > limit:
                tolerance = rules['preset']['Settings'].get(name, 'Default')
                rating = diffValue2txt(result.difficulty) if result.bool else 'unreachable'
                issue(f"{name} / {tolerance}: even with full equipment and ample ammo, VARIA rates this fight {rating}, above Maximum difficulty / {options['maxDifficulty']}. Raise Maximum difficulty or change this boss tolerance.", 8, 1)
    return dict(ok=not issues, issues=issues, notes=notes)


def validate_json(request_json, attempt=0):
    try:
        return json.dumps(check_settings(json.loads(request_json)), sort_keys=True)
    except Exception as ex:
        return json.dumps(dict(ok=False, issues=[dict(message=f'Settings check failed: {type(ex).__name__}: {ex}', page=0, otherPage=None)], notes=[]))
