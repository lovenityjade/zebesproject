#!/usr/bin/env python3
"""Read-only solver regression from a captured live request. No game/save writes."""
import copy,json,subprocess,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1]
query=json.loads(Path(sys.argv[1]).read_text())
worker="import sys;sys.path.insert(0,sys.argv[1]);from sm_live_tracker import evaluate_json;print(evaluate_json(sys.stdin.read()))"
queries=0
def evaluate(q):
 global queries
 queries+=1
 r=json.loads(subprocess.check_output([sys.executable,'-c',worker,str(root/'Randomizer')],input=json.dumps(q).encode()))
 assert r['ok'],r
 assert len(r['checks'])==100 and len(r['bosses'])==10
 assert [b['index'] for b in r['bosses']]==list(range(10))
 assert all('item' not in b and 'itemName' not in b for b in r['bosses'])
 return r
r=evaluate(query)
assert r['bosses'][5]['name']=='Spore Spawn' and r['bosses'][5]['state']==1
assert next(c for c in r['checks'] if c['name']=='Super Missile (pink Brinstar)')['state']==2
q=copy.deepcopy(query);q['inventory']['bosses'][1]|=2
s=evaluate(q)
assert s['bosses'][5]['state']==0
assert next(c for c in s['checks'] if c['name']=='Super Missile (pink Brinstar)')['state']==1
# More health alone does not fix the original reported dead end.
q=copy.deepcopy(query);q['inventory'].update(health=499,reserve=100)
assert [c['index'] for c in evaluate(q)['checks'] if c['state']==1]==[c['index'] for c in r['checks'] if c['state']==1]
# No imaginary boss kill, collected check, or placement may leak into inventory.
assert evaluate(query)==r
q=copy.deepcopy(query);q['placements']=[{'location':'Bomb','item':'SpaceJump'}]
assert evaluate(q)==r
# All combat markers, including Draygon's victory-dependent exit.
full=copy.deepcopy(query)
full['inventory'].update(items=0xf32f,beams=0x100f,health=1499,reserve=400,missiles=230,supers=50,powerBombs=50,bosses=[0]*8,collected=[0]*100)
full['settings']['maximumDifficulty']=1000000
all_result=evaluate(full)
assert all(b['state']==1 for b in all_result['bosses'] if b['name'] not in ('Mother Brain','Draygon')),all_result['bosses']
assert all_result['bosses'][2]['state']==2  # Botwoon blocks the normal route.
full['inventory']['bosses'][4]|=2
assert evaluate(full)['bosses'][2]['state']==1
# MB still needs the four major bosses; each encounter observes its own SRAM bit.
for i,(area,mask) in enumerate([(1,1),(3,1),(4,1),(2,1),(5,2),(1,2),(2,2),(4,2),(2,4),(0,4)]):
 q=copy.deepcopy(full);q['inventory']['bosses'][area]|=mask
 assert evaluate(q)['bosses'][i]['state']==0,(i,area,mask)
q=copy.deepcopy(full)
for area in (1,2,3,4):q['inventory']['bosses'][area]|=1
assert evaluate(q)['bosses'][4]['state']==1
# No weapons => no early accessible fight; changing combat settings is effective.
q=copy.deepcopy(query);q['inventory'].update(items=0,beams=0,health=99,reserve=0,missiles=0,supers=0,powerBombs=0,doors=[0]*64,collected=[0]*100)
assert not any(b['state']==1 for b in evaluate(q)['bosses'])
q=copy.deepcopy(full);q['settings']['maximumDifficulty']=0
assert evaluate(q)['bosses'][0]['state']!=1
# Effective boss routing matters: a shuffled Kraid entrance does not grant
# whichever combat used to occupy that entrance.
q=copy.deepcopy(query);pairs=q['topology']['bossPairs']
pairs[0][1],pairs[3][1]=pairs[3][1],pairs[0][1]
catalog=json.loads((root/'Randomizer/native_connections.json').read_text())
ids={ap['name']:ap['id'] for ap in catalog['accessPoints']};dest=[0]*len(ids)
for a,b in pairs:dest[ids[a]]=ids[b];dest[ids[b]]=ids[a]
q['topology'].update(mode='boss',native=dict(catalogSha256=catalog['sha256'],destinations=dest))
shuffled=evaluate(q)
assert shuffled['bosses'][0]['state']!=r['bosses'][0]['state']
# Tourian-disabled seeds must not advertise a nonexistent Mother Brain fight.
q=copy.deepcopy(full);q['settings']['tourian']='Disabled'
assert evaluate(q)['bosses'][4]['state']==255
print(json.dumps(dict(passed=True,queries=queries,liveSeedSporeSpawn='green before fight; gray after; reward then green',allTenEncounters=True,draygonVictoryExit=True,shuffledBossRouting=True,spoilerIndependent=True)))
