#!/usr/bin/env python3
"""Build the offline English editor catalog from the pinned, audited VARIA data."""
import json,re,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(ROOT/'Randomizer'))
from sm_options import ENUMS,TOGGLES,NUMBERS,SKILLS
src=json.loads((ROOT/'Docs/References/SMRandomizerUI/inventaires/settings.json').read_text())
# Include exclusions assembled programmatically by upstream, not just AST literals.
sys.path.insert(0,str(ROOT/'Randomizer/upstream'))
from utils.objectives import Objectives
for goal in src['objectives']:
 if goal['name'] in Objectives.goals:
  goal['constraints']['exclusion']=Objectives.goals[goal['name']].exclusion
 if goal['name']=='nothing':goal['category']='No additional objectives'
controls={x['id']:x for x in json.loads((ROOT/'Docs/References/SMRandomizerUI/Live-2026-09-15/controls.json').read_text())}
base={x['key']:x['default'] for x in src['randomizerDefaults'] if not isinstance(x['default'],dict)}
base.update(json.loads((ROOT/'Randomizer/upstream/rando_presets/vanilla.json').read_text()))
base.update(startLocation='Landing Site',progressionSpeed='medium',maxDifficulty='medium',morphPlacement='early',tourian='Vanilla',objective=['kill all G4'],objectiveRandom='false',hud='on',revealMap='on',better_reserves='on')
pages=[('Logic & difficulty','logic maxDifficulty'),('Progression','progressionSpeed progressionDifficulty morphPlacement suitsRestriction hideItems'),('Items & ammo','majorsSplit scavNumLocs scavRandomized energyQty minorQty missileQty superQty powerBombQty strictMinors funCombat funMovement funSuits'),('World & escape','startLocation areaRandomization bossRandomization doorsColorsRando allowGreyDoors minimizer minimizerQty escapeRando removeEscapeEnemies animals'),('Goals & victory','tourian objectiveRandom nbObjective nbObjectivesRequired hiddenObjectives distributeObjectives objective objectiveMultiSelect'),('Gameplay patches','gravityBehaviour nerfedCharge relaxed_round_robin_cf layoutPatches areaLayout variaTweaks hud revealMap itemsounds elevators_speed fast_doors spinjumprestart rando_speed Infinite_Space_Jump refill_before_save better_reserves raceMode')]
# Deliberate labels and help; never display raw backend identifiers as controls.
helptext={
'logic':('Game world','Original Super Metroid or mirrored world geometry.'),
'maxDifficulty':('Maximum difficulty','Hard ceiling for techniques, traversal and combat in generation and the live tracker. A skill preset alone is not an accessibility guarantee.'),
'progressionSpeed':('Progression speed','How early progression equipment appears. Random draws from the allowed choices below.'),
'progressionDifficulty':('Placement difficulty','Favor easier or harder routes within your allowed skill and difficulty settings.'),
'morphPlacement':('Morph Ball placement','Early, late or unrestricted placement of the Morph Ball.'),
'suitsRestriction':('Suit progression restriction','Restrict suit placement to keep Varia and Gravity part of progression.'),
'hideItems':('Hide items','Randomly conceal eligible visible pickups in shot blocks, as in VARIA.'),
'majorsSplit':('Item split','Full mixes all items. Major and Chozo restrict major equipment locations. Scavenger defines an ordered list of mandatory pickups.'),
'scavNumLocs':('Scavenger mandatory locations','Number of required locations in the scavenger route.'),
'scavRandomized':('Randomize scavenger items','Randomize the items at mandatory scavenger locations.'),
'energyQty':('Energy quantity','Amount of energy tanks and reserves available in the item pool.'),
'minorQty':('Ammo pack density','Percentage of ammo locations which contain ammo. Empty locations remain collectable checks.'),
'missileQty':('Missile weight','Relative Missile share of the ammo pool, from 1 to 9.'),
'superQty':('Super Missile weight','Relative Super Missile share of the ammo pool, from 1 to 9.'),
'powerBombQty':('Power Bomb weight','Relative Power Bomb share of the ammo pool, from 1 to 9.'),
'strictMinors':('Strict ammo proportions','Enforce the selected ammo ratios more strictly.'),
'funCombat':('Remove combat items','Challenge pool: remove nonessential combat equipment.'),
'funMovement':('Remove movement items','Challenge pool: remove nonessential movement equipment.'),
'funSuits':('Remove suits','Challenge pool: remove nonessential suits; your heat and suitless logic still applies.'),
'startLocation':('Starting location','Choose where Samus begins this seed.'),
'areaRandomization':('Area connections','Shuffle entrances between areas: Off, Light or Full.'),
'bossRandomization':('Boss connections','Shuffle the entrances to the four main bosses.'),
'doorsColorsRando':('Door colors','Randomize the ammo or beam required to open doors.'),
'allowGreyDoors':('Allow grey doors','Include locked grey doors in door color randomization.'),
'minimizer':('Minimizer','Build a smaller world. Enables mixed area/boss routing and disables suit restrictions. Below 100 locations, uses Full item placement.'),
'minimizerQty':('Minimizer locations','Target location count. Whole regions may yield more checks. Random chooses a target from 30 to 60.'),
'escapeRando':('Randomize escape','Randomize the final escape route and timer.'),
'removeEscapeEnemies':('Remove escape enemies','Remove enemies along the randomized escape route.'),
'animals':('Animals surprise','Randomize the surprise associated with saving the animals.'),
'objectiveRandom':('Draw random objectives','Select objectives from your eligible pool when Generate Game runs.'),
'nbObjective':('Objectives drawn','Number of objectives selected from the eligible pool.'),
'nbObjectivesRequired':('Objectives required','All, a fixed count, or a random count of the selected objectives.'),
'objective':('Selected objectives','Choose up to 18 goals. Mutually exclusive goals cannot be selected together.'),
'objectiveMultiSelect':('Eligible random objectives','Pool of goals from which the seed draws its objectives.'),
'hiddenObjectives':('Hidden objectives','Hide the selected objectives until discovered in the run.'),
'distributeObjectives':('Distribute objective categories','Balance random objectives across categories.'),
'tourian':('Finale after objectives','Vanilla Tourian, shortened Tourian, or return to the ship without Tourian.'),
'gravityBehaviour':('Suit behavior','Vanilla, balanced heat protection, or progressive protection from both suits.'),
'nerfedCharge':('Initial weakened Charge Beam','Start with a weak charge shot; the Charge pickup restores full damage.'),
'relaxed_round_robin_cf':('Flexible Crystal Flash','Use the relaxed round-robin ammunition requirements for Crystal Flash.'),
'layoutPatches':('Layout patches','Choose all, none, or individual traversal and escape adjustments.'),
'areaLayout':('Area layout patches','Choose individual room changes intended for area randomization.'),
'variaTweaks':('VARIA tweaks','Choose individual small gameplay changes.'),
'hud':('VARIA HUD','Area, check and objective information on the native HUD.'),
'revealMap':('Reveal map','Show room geometry. Exploration objectives must still count rooms actually visited.'),
'itemsounds':('Short item fanfare','Shorten the interruption when collecting an item.'),
'elevators_speed':('Fast elevators','Speed up elevator travel.'),
'fast_doors':('Fast door transitions','Speed up transitions between rooms.'),
'spinjumprestart':('Respin','Restart a spin jump while airborne.'),
'rando_speed':('Speedkeep','Preserve running speed through eligible transitions.'),
'Infinite_Space_Jump':('Infinite Space Jump','Relax the timing window for successive Space Jumps.'),
'refill_before_save':('Refill before saving','Seed-specific refill at a save station. The independent local comfort option remains available in Settings.'),
'better_reserves':('Better Reserves','Improved native reserve energy controls and display.'),
'raceMode':('Race mode','Protect seed spoilers for races; this is separate from the numeric seed.')}
fields=[]
for page,(_,keys) in enumerate(pages,1):
 for key in keys.split():
  label,detail=helptext[key]
  f=dict(key=key,page=page,label=label,description=detail,default=base.get(key,'off'))
  if key in ENUMS: f.update(type='choice',choices=[dict(value=x,label=x) for x in (*ENUMS[key],'random')])
  elif key in TOGGLES:f.update(type='choice',choices=[dict(value=x,label=x.title()) for x in ('off','on','random')])
  elif key in NUMBERS:
   f.update(type='number',minimum=NUMBERS[key][0],maximum=NUMBERS[key][1],decimal=key in ('missileQty','superQty','powerBombQty'))
   if f['default'] not in ('off','random'):f['default']=float(f['default']) if f['decimal'] else int(f['default'])
  elif key=='startLocation':f.update(type='choice',choices=[dict(value=x['name'],label=x['name']) for x in src['startAccessPointsVanilla']]+[dict(value='random',label='Random')])
  elif key=='objectiveRandom':f.update(type='choice',choices=[dict(value='false',label='Off'),dict(value='true',label='On')])
  else:f.update(type='goals',default=['kill all G4'] if key=='objective' else [g['name'] for g in src['objectives'] if g['available'] and g['name'] not in ('nothing','finish scavenger hunt')])
  c=controls.get(key,{}).get('options',[])
  for v in f.get('choices',[]):
   v['label']=next((x['label'] for x in c if x['value']==v['value']),v['label'].capitalize() if v['label'].islower() else v['label'])
  if key in src['multiselectValues'] and key!='objective':f['multiple']=key+'MultiSelect'
  if key in ('layoutPatches','areaLayout','variaTweaks'):
   f['type']='patches';group={'layoutPatches':'layout','areaLayout':'areaLayout','variaTweaks':'variaTweaks'}[key]
   f['custom']={'layoutPatches':'layoutCustom','areaLayout':'areaLayoutCustom','variaTweaks':'variaTweaksCustom'}[key]
   f['patches']=[x for x in src['patchSubselectors'] if x['group']==group]
  fields.append(f)
allowed={x['key'] for x in fields}|{x.get('multiple','') for x in fields}|{'layoutCustom','areaLayoutCustom','variaTweaksCustom'}
rpresets={}
visible_presets={v["value"] for v in controls["randoPreset"]["options"] if v["value"]}
for name,p in src['localPresetFiles']['rando_presets'].items():
 if name not in visible_presets:continue
 clean={k:v for k,v in p.items() if k in allowed}
 for k,v in list(clean.items()):
  if k in NUMBERS and v not in ('off','random'):clean[k]=float(v) if k in ('missileQty','superQty','powerBombQty') else int(v)
  if k=='objectiveRandom':clean[k]='true' if v in ('true','on',True) else 'false'
 # Mirror and Race were explicitly removed from the supported product scope.
 for removed in ('logic','logicMultiSelect','raceMode','raceModeMultiSelect'):clean.pop(removed,None)
 rpresets[name]=clean
skills={name:{k:v for k,v in p.items() if k in ('Knows','Settings')} for name,p in src['localPresetFiles']['standard_presets'].items()}
cat=dict(schema=1,pages=[p[0] for p in pages],fields=fields,techniques=src['techniques'],techniqueCategories=src['techniqueCategories'],techniqueOrder=list(src['techniqueCategories']),skillSettings=src['skillSettings'],objectives=[g for g in src['objectives'] if g.get('category') and g['available']],skillPresets=skills,settingsPresets=rpresets,settingsPresetSkills={n:p['preset'] for n,p in src['localPresetFiles']['rando_presets'].items() if p.get('preset') in SKILLS})
encoded=json.dumps(cat,ensure_ascii=True,separators=(',',':'))
target=ROOT/'Unreal/Source/SMUnreal/SMSeedCatalog.inl'
# MSVC limits individual string literals, even when the catalog is valid JSON.
# Separate array elements avoid both the token and concatenated-literal limits.
assert ')SMCAT"' not in encoded
chunks=[encoded[i:i+8000] for i in range(0,len(encoded),8000)]
target.write_text('// Generated by Scripts/build-seed-menu-catalog.py. No runtime web dependency.\nstatic const char* const SeedCatalogJsonParts[]={\n'+''.join('R"SMCAT('+chunk+')SMCAT",\n' for chunk in chunks)+'};\n')
print(f'{len(fields)} fields; {len(cat["techniques"])} techniques; {len(rpresets)} settings presets; {len(skills)} skill presets')

from logic.logic import Logic
Logic.factory('vanilla')
can_hide={loc.Address:bool(loc.CanHidden) for loc in Logic.locations() if not loc.isBoss()}
geometry=json.loads((ROOT/'Randomizer/native_locations.json').read_text())['locations']
(ROOT/'Native/sm_can_hide.inc').write_text('// Pinned VARIA eligible hidden pickup carriers, native address order.\n'+','.join('1' if can_hide[x['address']] else '0' for x in sorted(geometry,key=lambda x:x['address']))+'\n')
