#!/usr/bin/env python3
"""Effective all-18 quota and context reset; no source request re-normalization."""
import copy,json,os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
sys.path[:0]=[str(root/'Randomizer'),str(root/'Randomizer/upstream')]
from logic.logic import Logic
from utils.objectives import Objectives
from sm_objectives import configure_logic
Logic.factory('vanilla',new=True)
data=copy.deepcopy(json.loads((root/'objective-logic-seeds/seed-00.json').read_text())['manifest']['nativeContext']['objectives'])
catalog=json.loads((root/'Randomizer/native_objectives.json').read_text())
goals=[g for g in catalog['goals'] if g['kind']=='area_clear' or g['id'] in range(4) or g['name'] in ['kill spore spawn','kill botwoon','kill crocomire','kill golden torizo']]
assert len(goals)==18
data.update(goals=[g['id'] for g in goals],names=[g['name'] for g in goals],required=18,flags=3)
configure_logic(data,lambda:set())
assert [g.name for g in Objectives.activeGoals]==data['names']
assert Objectives.nbRequiredGoals==18 and Objectives.hidden
for goal in goals[:-1]:Objectives.setGoalCompleted(goal['name'],True)
assert not Objectives.enoughGoalsCompleted(),'All-18 quota was silently clamped'
Objectives.setGoalCompleted(goals[-1]['name'],True);assert Objectives.enoughGoalsCompleted()
configure_logic()
assert [g.name for g in Objectives.activeGoals]==Objectives.vanillaGoals
assert Objectives.nbRequiredGoals==4 and not Objectives.enoughGoalsCompleted()
assert not Objectives.hidden and not Objectives.permissive and Objectives.totalEnemies is None
(root/'objectives-logic/boundaries.json').write_text(json.dumps(dict(passed=True,effectiveQuota=18,refusedAt=17,acceptedAt=18,legacyContextReset=True),indent=2)+'\n')
print('OBJECTIVE_LOGIC_BOUNDARIES_PASS',flush=True)
