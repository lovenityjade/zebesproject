#!/usr/bin/env python3
"""Production objective requests with source solver and progression proof, gaming-pc only."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
out=root/'objective-public-seeds';out.mkdir(exist_ok=True)
cases=[
 dict(seed=15093000,options={},goals=['kill all G4'],required='off'),
 dict(seed=15093001,options={'majorsSplit':'FullWithHUD','areaRandomization':'full','revealMap':'off'},
      goals=['tickle the red fish','collect 25% items','explore 50% map'],required=2,hidden=True),
 dict(seed=15093002,options={'majorsSplit':'Chozo','hud':'off','hiddenObjectives':'on','distributeObjectives':'on'},
      goals=['clear green brinstar','visit the animals','activate chozo robots'],required='off'),
 dict(seed=15093003,options={'bossRandomization':'on','doorsColorsRando':'on'},
      goals=['collect all upgrades','kill all beetoms'],required='off'),
 dict(seed=15093004,options={},
      goals=['nothing'],required=1),
 dict(seed=15093005,options={'minorQty':7,'variaTweaks':'on'},goals=['collect 100% items'],required='off'),
 dict(seed=15093006,options={'minorQty':7,'variaTweaks':'on'},goals=['tickle the red fish','activate chozo robots','visit the animals'],required=1),
 dict(seed=15093007,options={},goals=['tickle the red fish','activate chozo robots','visit the animals'],required='random',hidden=True,draw='random'),
 dict(seed=15093008,options={},goals=['tickle the red fish','activate chozo robots','visit the animals'],required='random',hidden=True,draw=3)]
if len(sys.argv)==2:
 reports=[]
 for i,case in enumerate(cases):
  for attempt in range(8):
   subprocess.run([sys.executable,__file__,str(root),str(i),str(attempt)],check=True)
   result=json.loads((out/f'candidate-{i:02d}-{attempt}.json').read_text())
   if result.get('ok'):break
   assert result.get('retry'),result
  assert result.get('ok'),result
  (out/f'seed-{i:02d}.json').write_text(json.dumps(result,indent=2)+'\n')
  m=result['manifest'];v=m['solverVerification'];obj=m['nativeContext']['objectives']
  report=dict(seed=m['seed'],fingerprint=m['sha256'],checks=v['reachableItemCount'],
              nativeObjectives=obj,objectiveVerification=v['objectiveVerification'])
  reports.append(report)
  print('OBJECTIVE_PUBLIC_SEED_PASS',i,m['seed'],obj['names'],flush=True)
 (out/'verification.json').write_text(json.dumps(dict(seeds=reports,
   modules={name:hashlib.sha256((root/'Randomizer'/name).read_bytes()).hexdigest()
            for name in ('sm_options.py','sm_objectives.py','sm_integration.py','sm_validation.py','sm_tracker_data.py')},
   scope=__doc__),indent=2)+'\n')
 raise SystemExit(0)
i,attempt=map(int,sys.argv[2:]);case=cases[i]
sys.path[:0]=[str(root/'Randomizer'),str(root/'Randomizer/upstream')]
import sm_integration,sm_options
options=dict(case['options'],objective=case['goals'],nbObjectivesRequired=case['required'])
if case.get('hidden'):
 options.update(objectiveRandom='true',nbObjective=case.get('draw',len(case['goals'])),objectiveMultiSelect=case['goals'],hiddenObjectives='on',distributeObjectives='on')
request=dict(seed=case['seed'],skill='regular',options=options)
result=json.loads(sm_integration.generate_json(json.dumps(request),attempt))
(out/f'candidate-{i:02d}-{attempt}.json').write_text(json.dumps(result,indent=2)+'\n')
if not result.get('ok'):
 print('OBJECTIVE_PUBLIC_CANDIDATE_REJECTED',i,attempt,result,flush=True)
 raise SystemExit(0)
m=result['manifest'];v=m['solverVerification'];obj=m['nativeContext']['objectives'];proof=v['objectiveVerification']
assert m['requestedSettings']['options']==options
effective=m['rules']['options']
assert effective['objective']==obj['names'] and effective['nbObjective']==len(obj['goals'])
assert effective['nbObjectivesRequired']==obj['required']
assert (effective['hiddenObjectives']=='on')==bool(obj['flags']&2)
if i==1:assert effective['revealMap']=='on'
if i==2:assert effective['hud']=='on' and effective['hiddenObjectives']=='off' and effective['distributeObjectives']=='off'
bomb=next(p for p in m['placements'] if p['location']=='Bomb')
assert bool(obj['flags']&4)==bool(case['options'].get('variaTweaks')=='on' and bomb['kind']==2 and 'activate chozo robots' not in obj['names'])
expected=['kill kraid','kill phantoon','kill draygon','kill ridley'] if i==0 else case['goals']
if case.get('draw')=='random':
 assert 1<=len(obj['names'])<=len(expected) and set(obj['names'])<=set(expected)
else:assert set(obj['names'])==set(expected),(obj,expected)
assert proof['names']==obj['names'] and proof['required']==obj['required']
if case['required']=='random':assert 1<=obj['required']<=len(obj['goals'])
else:assert obj['required']==(len(expected) if case['required']=='off' else case['required'])
assert v['allItemsReachable'] and v['completionVerified'] and v['reachableItemCount']==100
assert proof['quotaReached'] and len(proof['completed'])>=obj['required']
assert len(set(proof['completed']))==len(proof['completed']) and set(proof['completed'])<=set(expected)
assert m['tracker']['settings']['nativeObjectives']==obj
assert m['tracker']['settings']['goals']==obj['names']+['MotherBrain','escapeToShip']
assert bool(obj['flags']&2)==bool(case.get('hidden'))
from logic.logic import Logic
locations=sorted((l for l in Logic.locations() if not l.isBoss()),key=lambda l:l.Address)
from utils.objectives import Objectives
for entry in proof['progressionLog']:
 name=entry['objectiveName'];visited=set(entry['collectedChecksBefore'])
 if name.startswith('collect ') and '% items' in name:
  pct=int(name.split()[1].rstrip('%'))
  have=sum(flag and loc.Name in visited for flag,loc in zip(obj['itemCounted'],locations))
  assert have>=(sum(obj['itemCounted'])*pct+99)//100
 if name.startswith('clear '):
  area=Objectives.goals[name].area
  assert all(loc.Name in visited for loc,flag in zip(locations,obj['areaCounted']) if flag and loc.GraphArea==area)
 if name=='collect all upgrades':
  from rando.Items import ItemManager
  items=beams=0
  for item in ItemManager.Items.values():
   if entry['inventoryBefore'].get(item.Type):items|=item.ItemBits;beams|=item.BeamBits
  assert (items,beams)==(obj['itemMask'],obj['beamMask'])
if i==4:
 first=next(e for e in proof['progressionLog'] if e['routeStep']==proof['quotaRouteStep'])
 assert first['objectiveName']=='nothing' and first['completedCount']==1 and first['afterLocationStep']==0
if i==5:assert sum(obj['itemCounted'])<100
# Collection predicates must not be satisfied by a cheated equipment inventory.
# Conversely each counted location counts once, even with duplicate ammo/items.
from sm_objectives import configure_logic
from logic.smboolmanager import SMBoolManager
from rando.Items import ItemManager
sm=SMBoolManager()
for item in ItemManager.Items.values():
 if item.Category!='Nothing':sm.addItem(item.Type)
collected=set();configure_logic(obj,lambda:collected)
for name in obj['names']:
 if name.startswith('collect ') and '% items' in name:
  pct=int(name.split()[1].rstrip('%'));threshold=(sum(obj['itemCounted'])*pct+99)//100
  names=[loc.Name for loc,flag in zip(locations,obj['itemCounted']) if flag]
  collected.clear();collected.update(names[:threshold-1])
  assert not Objectives.goals[name].canClearGoal(sm,'Landing Site')
  collected.update(loc.Name for loc,flag in zip(locations,obj['itemCounted']) if not flag)
  assert not Objectives.goals[name].canClearGoal(sm,'Landing Site')
  collected.add(names[threshold-1]);assert Objectives.goals[name].canClearGoal(sm,'Landing Site')
 if name.startswith('clear '):
  goal=Objectives.goals[name]
  names={loc.Name for loc,flag in zip(locations,obj['areaCounted']) if flag and loc.GraphArea==goal.area}
  assert names,'Choose a region with counted items for the boundary fixture'
  collected.clear();collected.update(loc.Name for loc in locations if loc.Name not in names)
  assert not goal.canClearGoal(sm,'Landing Site')
  collected.update(names);assert goal.canClearGoal(sm,'Landing Site')
