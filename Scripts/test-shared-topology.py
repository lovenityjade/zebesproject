#!/usr/bin/env python3
"""Native embedded oracle over all boss mappings and fresh generated seeds."""
import ctypes as C,itertools,json,os,sys,copy
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
sys.path[:0]=[str(root/'Randomizer'),str(root/'Randomizer/upstream')]
from logic.logic import Logic
from graph.graph import AccessGraph
from graph.graph_utils import GraphUtils,vanillaTransitions,vanillaEscapeTransitions,vanillaBossesTransitions
from sm_topology import capture,validate,vanilla
l=C.CDLL(str(root/'connections/libsm_native.so'))
l.sm_tracker_evaluate.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
l.sm_randomizer_generate.argtypes=l.sm_tracker_evaluate.argtypes
catalog=json.loads((root/'Randomizer/native_connections.json').read_text());aps=catalog['accessPoints']
inside=[p['name'] for p in aps if p['inside']];outside=[p['name'] for p in aps if not p['inside']]
m=json.loads((root/'results/seed-14092026.json').read_text())['manifest'];base_tracker=m['tracker']
lookups={item['type']:item for item in base_tracker['items']}
def invoke(method,q):
 b=C.create_string_buffer(2097152);n=method(str(root/'Randomizer').encode(),json.dumps(q).encode(),b,len(b));assert 0<n<=len(b);return json.loads(b.value)
results=[];signatures=set()
for perm in itertools.permutations(inside):
 pairs=list(zip(outside,perm));Logic.factory('vanilla',new=True)
 graph=AccessGraph(Logic.accessPoints(),vanillaTransitions+pairs+[vanillaEscapeTransitions[0]])
 doors=GraphUtils.getDoorConnections(graph,False,True,False)
 topology=capture(dict(boss=True,area=False,doors=doors,escapeAttr=None))
 assert {frozenset(p) for p in topology['bossPairs']}=={frozenset(p) for p in pairs}
 assert len(validate(topology))==len(vanillaTransitions)+5
 configs=[]
 for equipment in [('Morph','Bomb','HiJump','Varia'),('Morph','Bomb','SpaceJump')]:
  inv=dict(items=0,beams=0,health=799,reserve=0,missiles=25,supers=10,powerBombs=0,bosses=[1]*8,doors=[0]*64,collected=[0]*100)
  for name in equipment:inv['items']|=lookups[name]['itemBits'];inv['beams']|=lookups[name]['beamBits']
  t=copy.deepcopy(base_tracker['topology']);t.update(topology)
  r=invoke(l.sm_tracker_evaluate,dict(randomized=True,settings=base_tracker['settings'],topology=t,inventory=inv))
  assert r['ok'] and r['topologyMode']=='boss' and r['bossConnections']==topology['bossPairs'],r
  state=tuple(c['state'] for c in r['checks']);configs.append(state);signatures.add((equipment,state))
 results.append(dict(topology=topology,states=configs))
 print('SHARED_TOPOLOGY_PASS',len(results),flush=True)
assert len(signatures)>2,'All boss permutations incorrectly reused one accessibility result'
invalid=[]
for edit in ['duplicate','same-role','unknown','wrong-mode','area-change','missing-escape','native-mismatch','catalog-mismatch']:
 t=copy.deepcopy(results[-1]['topology'])
 if edit=='duplicate':t['bossPairs'][1]=t['bossPairs'][0]
 elif edit=='same-role':t['bossPairs']=[[outside[0],outside[1]],[inside[0],inside[1]],[outside[2],inside[2]],[outside[3],inside[3]]]
 elif edit=='unknown':t['bossPairs'][0][0]='unknown'
 elif edit=='wrong-mode':t['mode']='area'
 elif edit=='area-change':t['areaPairs'][0][0]='unknown'
 elif edit=='native-mismatch':t['native']['destinations']=[0]*8
 elif edit=='catalog-mismatch':t['native']['catalogSha256']='0'*64
 else:del t['escapePairs']
 try:validate(t)
 except ValueError:invalid.append(edit)
 else:raise AssertionError(edit)
out=root/'connections';seeds=[]
for i in range(6):
 q=dict(seed=15092400+i,skill='regular',noAdvancedTechs=bool(i%2),options=dict(layoutPatches='on' if i%2 else 'off',variaTweaks='on',bossRandomization='on' if i>=2 else 'off'))
 r=invoke(l.sm_randomizer_generate,q);assert r['ok'],r
 manifest=r['manifest'];v=manifest['solverVerification']
 assert v['allItemsReachable'] and v['completionVerified'] and v['topologyMode']==('boss' if i>=2 else 'vanilla')
 assert manifest['nativeContext']['topology']['bossPairs']==manifest['tracker']['topology']['bossPairs']==v['bossConnections']
 target=root/'boss-seeds' if i>=2 else out;target.mkdir(exist_ok=True)
 (target/(f'seed-{i-2:02d}.json' if i>=2 else f'vanilla-seed-{i}.json')).write_text(json.dumps(r,indent=2));seeds.append(dict(seed=manifest['seed'],checks=v['reachableItemCount'],topology=manifest['nativeContext']['topology']))
 print('GENERATED_TOPOLOGY_PASS',i,manifest['seed'],v['topologyMode'],flush=True)
assert len({tuple(s['topology']['native']['destinations']) for s in seeds[2:]})>=2
(out/'topology-verification.json').write_text(json.dumps(dict(permutations=results,queries=48,distinctResults=len(signatures),rejections=invalid,generatedSeeds=seeds),indent=2))
