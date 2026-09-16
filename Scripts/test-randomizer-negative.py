#!/usr/bin/env python3
import copy,json,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
sys.path[:0]=[str(ROOT/'Randomizer'),str(ROOT/'Randomizer/upstream')]
from sm_validation import verify_progression
m=json.loads((ROOT/'Docs/Randomizer/seed-14092026.json').read_text())['manifest']
checks=[]
for field,value in [('item','Gravity'),('address',0),('visibility','Invalid')]:
    items=copy.deepcopy(m['placements']);items[0][field]=value
    try: verify_progression(items,m['skill'],m['logicPatches'])
    except ValueError:checks.append(field+' mismatch rejected')
    else:raise AssertionError('Corrupted spoiler accepted: '+field)
# A syntactically valid table with no Morph/abilities must fail progression.
items=copy.deepcopy(m['placements'])
for p in items:
    p['item']='Missile';p['plm']=0xeedb+{'Visible':0,'Chozo':84,'Hidden':168}[p['visibility']]
bad=verify_progression(items,m['skill'],m['logicPatches'])
assert not bad['allItemsReachable'] and not bad['motherBrainDefeated']
checks.append('valid PLMs with impossible progression rejected')
report=dict(passed=True,checks=checks,impossibleSeedReachable=bad['reachableItemCount'],impossibleSeedUnavailable=bad['unavailable'])
(ROOT/'Docs/Randomizer/negative-verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('SM_RANDOMIZER_NEGATIVE_PASS',checks)
