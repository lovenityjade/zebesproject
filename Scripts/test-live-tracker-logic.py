#!/usr/bin/env python3
"""End-to-end embedded tracker queries, no emulator or PopTracker dependency."""
import ctypes as C, copy, hashlib, json, os, sys, time
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
assert not (root/'Docs/References/PopTracker/pack').exists()
out=root/'tracker-results';out.mkdir(exist_ok=True)
lib=C.CDLL(str(root/'Native/build/libsm_native.so'))
fn=lib.sm_tracker_evaluate;fn.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
base=dict(items=0,beams=0,health=99,reserve=0,missiles=0,supers=0,powerBombs=0,bosses=[0]*8,doors=[0]*64,collected=[0]*100)
queries=0;seconds=0

def evaluate(q):
 global queries,seconds
 b=C.create_string_buffer(262144);t=time.monotonic()
 n=fn(str(root/'Randomizer').encode(),json.dumps(q).encode(),b,len(b));queries+=1;seconds+=time.monotonic()-t
 assert 0<n<=len(b),n
 result=json.loads(b.value);assert result.get('ok'),result
 assert len(result['checks'])==100 and len({c['address'] for c in result['checks']})==100
 assert [c['index'] for c in result['checks']]==list(range(100))
 assert all('item' not in c and 'itemName' not in c for c in result['checks'])
 return result

def request(t,inv):return dict(randomized=True,settings=t['settings'],topology=t['topology'],inventory=inv)
def from_inventory(t,items,visited):
 inv=copy.deepcopy(base)
 for bit in t['settings'].get('initialDoors',[]):inv['doors'][bit//8]|=1<<(bit%8)
 for item in t['items']:
  if items.get(item['type'],0):inv['items']|=item['itemBits'];inv['beams']|=item['beamBits']
 inv.update(health=99+100*items.get('ETank',0),reserve=100*items.get('Reserve',0),
  missiles=5*items.get('Missile',0),supers=5*items.get('Super',0),powerBombs=5*items.get('PowerBomb',0))
 for name,area,mask in [('Kraid',1,1),('Phantoon',3,1),('Draygon',4,1),('Ridley',2,1),('MotherBrain',5,2),('SporeSpawn',1,2),('Crocomire',2,2),('Botwoon',4,2),('GoldenTorizo',2,4)]:
  if items.get(name,0):inv['bosses'][area]|=mask
 inv['collected']=[int(loc['name'] in visited) for loc in t['locations']]
 return inv

v=evaluate(dict(randomized=False,inventory=base))
assert [c['name'] for c in v['checks'] if c['state']==1]==['Morphing Ball']
reports=[]
for seed in [14092026,14092027,14092028]:
 m=json.loads((root/f'results/seed-{seed}.json').read_text())['manifest'];t=m['tracker'];visited=set();cases=[]
 initial=evaluate(request(t,base));assert initial['maximumDifficulty']==t['settings']['maximumDifficulty']
 assert {c['name'] for c in initial['checks'] if c['state']==1}=={'Morphing Ball','Energy Tank, Brinstar Ceiling'}
 byname={c['name']:i for i,c in enumerate(initial['checks'])}
 # Every actual next location from the previously verified solver progression.
 # Unknown placed items may be needed for a return: yellow is truthful here.
 for p in m['solverVerification']['progressionLog']:
  inv=from_inventory(t,p['inventoryBefore'],visited)
  if p['location'] in byname:
   r=evaluate(request(t,inv));c=r['checks'][byname[p['location']]]
   assert c['state'] in (1,4),(seed,p['step'],p['location'],c)
   assert c['reachable'],(seed,p['step'],c)
   for check in r['checks']:
    if check['name'] in visited:assert check['state']==0
   cases.append(dict(step=p['step'],location=p['location'],state=c['state'],reason=c['reason']))
  visited.add(p['location'])
 assert len(cases)==100
 # Native capacities, not current ammo; boss state independently supplied.
 all_items={i['type']:1 for i in t['items']};all_items.update(ETank=14,Reserve=4,Missile=46,Super=10,PowerBomb=10)
 inv=from_inventory(t,all_items,set());all_result=evaluate(request(t,inv))
 assert all(c['state']==1 for c in all_result['checks']),[(c['name'],c['state'],c['reason']) for c in all_result['checks'] if c['state']!=1]
 # Collection belongs to a location even when its received item is not owned.
 inv=copy.deepcopy(base);inv['collected'][0]=1
 assert evaluate(request(t,inv))['checks'][0]['state']==0
 # Spoiler input cannot grant an unknown item / repair the return route.
 q=request(t,base);q['placements']=m['placements'];q['placements'][0]['item']='SpaceJump'
 assert evaluate(q)==initial
 reports.append(dict(seed=seed,skill=m['skill'],progression=m['progression'],steps=cases))
 print('PROGRESSION PASS',seed,len(cases),flush=True)

# Expanded settings really control access (no silent preset/default fallback).
t=copy.deepcopy(t);inv=from_inventory(t,dict(Morph=1,Bomb=1,Missile=5,Super=3,PowerBomb=2,ETank=8,Varia=1),set())
on=copy.deepcopy(t);off=copy.deepcopy(t)
for value in on['settings']['knows'].values():value.update(enabled=True,difficulty=1)
for value in off['settings']['knows'].values():value['enabled']=False
a=evaluate(request(on,inv));b=evaluate(request(off,inv))
different=[x['name'] for x,y in zip(a['checks'],b['checks']) if x['state']!=y['state']]
assert different,'Techniques ignored'
# Restore a former context after many unrelated presets: isolated interpreter.
assert evaluate(dict(randomized=False,inventory=base))==v
report=dict(passed=True,host=os.uname().nodename,packAbsent=True,queries=queries,querySeconds=seconds,
 seeds=reports,expandedTechniqueDifferences=different,spoilerIndependence=True,vanillaBaselineRestored=True,
 nativeSha256=hashlib.sha256((root/'Native/build/libsm_native.so').read_bytes()).hexdigest())
(out/'logic-verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('LIVE TRACKER LOGIC PASS',queries,seconds,flush=True)
