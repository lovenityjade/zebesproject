#!/usr/bin/env python3
"""Generated hunt orders and objective quotas through native pickup/save routines."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
exec(compile((root/'test-scavenger-native.py').read_text().split('cases=0')[0],'scavenger-fixture','exec'))
reports=[]
for slot,file in enumerate(sorted((root/'scavenger/public').glob('seed-*.json'))):
 manifest=json.loads(file.read_text())['manifest'];context=manifest['nativeContext'];h=context['scavenger'];o=context['objectives']
 hp=Hunt();hp.version=1;hp.size=C.sizeof(hp);hp.count=len(h['words']);hp.order[:hp.count]=h['words']
 assert configure_hunt(slot,C.byref(hp),h['catalogSha256'].encode())
 p=plan(o['goals'],o['required'],int(o['areaLayout']),o['flags']);p.version=2
 p.item_mask=o['itemMask'];p.beam_mask=o['beamMask']
 p.item_counted[:]=o['itemCounted'];p.area_counted[:]=o['areaCounted']
 p.map_totals[:]=o['mapTotals'];p.enemy_totals[:]=o['enemyTotals']
 # Source item placements, no substitutions. Area mapping is configured to
 # validate totals; world traversal is covered by the Unreal seed fixture.
 items=(Item*100)(*[Item(x['address'],x['plm'],x.get('kind',0)) for x in manifest['placements']])
 m=manifest
 commit(p,slot,int(o['areaLayout']));clear();new_game()
 goal=o['goals'].index(16)
 for index,entry in enumerate(h['words']):
  assert value(goal,1)==index and value(goal,2)==len(h['words']) and not value(goal,4)
  for later in h['words'][index+1:]:assert not pick(later>>8)
  assert pick(entry>>8)
  if entry>>8==0xaa:mark(80)
  for _ in range(p.count):frame()
  snap=objective_snapshot();assert snap.values[goal][1]==index+1
  save(slot);put(0xd8f4,0);assert load(slot)==0 and st(1)==index+1
 assert value(goal,0) and value(goal,4) and st(3)
 # Clearing the hunt dependency cannot activate an objective-v2 slot.
 assert configure_hunt(slot,None,h['catalogSha256'].encode())
 assert not activate(slot,1) and st(3)
 assert configure_hunt(slot,C.byref(hp),h['catalogSha256'].encode())
 reports.append(dict(seed=manifest['seed'],fingerprint=manifest['sha256'],slot=slot,steps=hp.count))
assert l.sm_cpu_opcodes()==0
(root/'scavenger/objectives-results.json').write_text(json.dumps(dict(passed=True,cases=reports,cpuOpcodes=0,nativeSha256=hashlib.sha256((root/'scavenger/libsm_native.so').read_bytes()).hexdigest(),scope=__doc__),indent=2)+'\n')
l.sm_shutdown();print('SCAVENGER_OBJECTIVES_PASS',flush=True)
