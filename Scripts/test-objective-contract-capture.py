#!/usr/bin/env python3
"""Capture effective VARIA objective data from real generated item pools.

Stops before native seed activation/verification. This proves serialization,
not objective-aware solver or tracker integration (which remains unfinished).
"""
import copy,hashlib,json,os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
if len(sys.argv)==2:
    import subprocess
    gathered=[]
    for index in range(3):
        subprocess.run([sys.executable,str(Path(__file__).resolve()),str(root),str(index)],check=True)
        gathered.extend(json.loads((root/'objectives'/f'capture-{index}.json').read_text())['cases'])
    report=dict(cases=gathered,note='Fresh-process generated pools and authoritative VARIA writers; not enabled native objective-aware seed generation',moduleSha256=hashlib.sha256((root/'Randomizer/sm_objectives.py').read_bytes()).hexdigest())
    (root/'objectives/capture-results.json').write_text(json.dumps(report,indent=2)+'\n')
    raise SystemExit(0)
case_index=int(sys.argv[2])
sys.path[:0]=[str(root/'Randomizer/upstream'),str(root/'Randomizer')]
sys.argv[0]=str(root/'Randomizer/upstream/randomizer.py')
import sm_integration,sm_native_world,sm_options,sm_objectives
from utils.objectives import Objectives
from rom.addresses import Addresses
from graph.graph_utils import graphAreas
class Captured(BaseException):pass
old_resolve=sm_options.resolve;old_world=sm_native_world.capture
cases=[
 dict(seed=15092710,options={},goals=['kill all G4'],required='off',hidden=False),
 dict(seed=15092711,options={'majorsSplit':'FullWithHUD','areaRandomization':'full'},goals=['tickle the red fish','collect 25% items','explore 50% map'],required=2,hidden=True),
 dict(seed=15092712,options={'majorsSplit':'Chozo'},goals=['visit the animals','activate chozo robots','kill all beetoms'],required='off',hidden=False)]
cases=[cases[case_index]]
results=[]
for case in cases:
    def resolve(request):
        rules=old_resolve(request)
        # Fixture-only access to source generator settings. Production guards
        # deliberately stay enabled until menu/solver/tracker activation exists.
        rules['options'].update(objective=case['goals'],nbObjectivesRequired=case['required'],hiddenObjectives='on' if case['hidden'] else 'off')
        if case['hidden']:rules['options'].update(objectiveRandom='true',nbObjective=len(case['goals']),objectiveMultiSelect=case['goals'])
        return rules
    def capture(patcher,rules):
        before=copy.deepcopy(patcher.romFile.data)
        data=sm_objectives.capture(patcher)
        assert patcher.romFile.data==before
        assert data['names']==[g.name for g in Objectives.activeGoals]
        expected=['kill kraid','kill phantoon','kill draygon','kill ridley'] if case['goals']==['kill all G4'] else case['goals']
        assert set(data['names'])==set(expected),(data['names'],expected)
        assert data['required']==(len(data['goals']) if case['required']=='off' else case['required'])
        assert bool(data['flags']&2)==case['hidden']
        # Compare each count-list mask against actual source-written location IDs.
        itemlocs={il.Location.Address:il for il in patcher.settings['itemLocs'] if not il.Location.isBoss()}
        locations=sorted(json.loads((root/'Randomizer/native_locations.json').read_text())['locations'],key=lambda l:l['address'])
        source_ids=set()
        for area in graphAreas:
            addr=Addresses.getOne('objectives_locs_'+area)
            for i in range(101):
                value=before[addr+i]
                if value==255:break
                source_ids.add(value)
        assert data['areaCounted']==[int(itemlocs[l['address']].Location.Id in source_ids) for l in locations]
        # Actual item writer independently computes total excluding Nothing/restricted.
        patcher.writeItemsLocs(patcher.settings['itemLocs'])
        assert sum(data['itemCounted'])==patcher.nItems
        assert sum(data['areaCounted'])<=sum(data['itemCounted'])
        for field,bad in [('required',0),('flags',8),('catalogSha256','0'*64),('mapTotals',[0]*12),('enemyTotals',[1]*6),('goals',[59]),('itemCounted',[2]*100)]:
            wrong=copy.deepcopy(data);wrong[field]=bad
            try:sm_objectives.validate(wrong)
            except ValueError:pass
            else:raise AssertionError('Accepted invalid '+field)
        results.append(dict(seed=case['seed'],split=patcher.settings['majorsSplit'],contract=data))
        raise Captured()
    sm_options.resolve=resolve;sm_native_world.capture=capture
    try:sm_integration.generate_once(dict(seed=case['seed'],skill='casual',options=case['options']))
    except Captured:pass
    finally:sm_options.resolve=old_resolve;sm_native_world.capture=old_world
    assert len(results)==cases.index(case)+1
    print('OBJECTIVE_CONTRACT_CAPTURE_PASS',case['seed'],flush=True)
(root/'objectives'/f'capture-{case_index}.json').write_text(json.dumps(dict(cases=results,note='Generated pools and authoritative VARIA writers; not enabled native objective-aware seed generation',moduleSha256=hashlib.sha256((root/'Randomizer/sm_objectives.py').read_bytes()).hexdigest()),indent=2)+'\n')
