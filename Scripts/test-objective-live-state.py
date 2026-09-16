#!/usr/bin/env python3
"""Native objective evidence -> embedded oracle -> stale-safe publication.

Uses a generated goal/placement contract and controlled RAM/events. This is
not a physical seven-seed playthrough or a rendered objectives screen.
"""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-objectives.py').read_text().split("for goal in audit['goals']:")[0]
source=source.replace('objectives/libsm_native.so','objectives-logic/libsm_native.so').replace("root/'objectives/native'","root/'objectives-logic/native'")
exec(compile(source,'native-objective-state-fixture','exec'))
out=root/'objectives-logic/live';out.mkdir(exist_ok=True)
m=json.loads((root/'objective-logic-seeds/seed-06.json').read_text())['manifest'];t=m['tracker'];data=m['nativeContext']['objectives']
items=(Item*100)(*[Item(p['address'],p['plm'],p.get('kind',0)) for p in m['placements']])
p=plan(data['goals'],data['required'],flags=data['flags'])
p.item_counted[:]=data['itemCounted'];p.area_counted[:]=data['areaCounted']
p.item_mask=data['itemMask'];p.beam_mask=data['beamMask']
commit(p);clear()
l.sm_tracker_publish_objectives.argtypes=[C.POINTER(Snapshot),C.POINTER(ObjectiveSnapshot),C.POINTER(C.c_uint8),C.c_uint32]
l.sm_tracker_configure(1,1,1)
def request(s,o=None):
    inv=dict(items=s.acquired_items,beams=s.acquired_beams,health=s.max_health,reserve=s.max_reserve,
      missiles=s.max_missiles,supers=s.max_supers,powerBombs=s.max_power_bombs,bosses=list(s.boss_bits),
      doors=list(s.opened_doors),collected=list(s.collected))
    q=dict(randomized=True,settings=t['settings'],topology=t['topology'],inventory=inv)
    if o is not None:q['objectiveState']=dict(version=o.version,state=list(o.state),goals=list(o.goals),values=[list(row) for row in o.values])
    return q
def evaluate(q,ok=True):
    buf=C.create_string_buffer(262144)
    n=l.sm_tracker_evaluate(str(root/'Randomizer').encode(),json.dumps(q).encode(),buf,len(buf))
    assert 0<n<=len(buf);r=json.loads(buf.value);assert r['ok']==ok,r
    return r
def publish(s,o,r):
    states=(C.c_uint8*100)(*[c['state'] for c in r['checks']])
    return l.sm_tracker_publish_objectives(C.byref(s),C.byref(o),states,100)
# Owning everything makes the interaction possible, but cannot complete it.
put(0x9a4,0xf32f);put(0x9a8,0x100f);put(0x9c4,1499);put(0x9c8,230);put(0x9cc,50);put(0x9d0,50);put(0x9d4,400)
s=snap();o=objective_snapshot();r=evaluate(request(s,o))
fish=next(i for i,g in enumerate(data['names']) if g=='tickle the red fish')
assert r['objectives']['goals'][fish]['completable']
assert not r['objectives']['goals'][fish]['completed'] and r['objectives']['completedCount']==0
assert publish(s,o,r)
missing=evaluate(request(s))['objectives']
assert missing['progressSource']=='unavailable' and missing['completedCount'] is None
assert all(g['completed'] is None for g in missing['goals'])
# ZOE1-only change: inventory and original events have not changed at all.
mark(audit['goals'][data['goals'][fish]]['arg'])
after=snap();assert bytes(s.events)==bytes(after.events) and bytes(s.boss_bits)==bytes(after.boss_bits)
assert not publish(s,o,r),'Accepted stale interaction evidence'
assert l.sm_tracker_area_count(0,1)==-1,'Rendered stale logical colors'
raw=objective_snapshot();assert raw.values[fish][4]==1 and raw.values[fish][0]==0
pre=evaluate(request(after,raw))['objectives']['goals'][fish]
assert pre['conditionMet'] and not pre['completed']
for _ in range(p.count):frame()
s=snap();o=objective_snapshot();r=evaluate(request(s,o))
assert r['objectives']['goals'][fish]['completed'] and r['objectives']['requiredMet']
assert r['objectives']['completedCount']==1 and not r['objectives']['allMet']
assert publish(s,o,r) and l.sm_tracker_area_count(0,1)>=0
# Original checksummed save/load persists observed completion, not feasibility.
put(0x952,0);save(0);C.memset(ram+0xd91c+0x180,0,37);assert not event(162+2*fish)
assert load(0)==0
loaded=evaluate(request(snap(),objective_snapshot()))
assert loaded['objectives']['goals'][fish]['completed']
# Matching goal count alone cannot make another seed's goal IDs valid.
bad=request(snap(),objective_snapshot());bad['objectiveState']['goals'][fish]=0
assert 'does not match' in evaluate(bad,False)['error']
# State/slot/session and companion changes both invalidate delayed responses.
s=snap();o=objective_snapshot();r=evaluate(request(s,o));put(0x952,1)
assert not publish(s,o,r);put(0x952,0)
assert select(None,0,None)
inactive=objective_snapshot();assert inactive.state[0]==0
assert not publish(s,o,r)
(out/'verification.json').write_text(json.dumps(dict(passed=True,seed=m['seed'],fingerprint=m['sha256'],
    before=r['objectives'],saved=loaded['objectives'],source=__doc__,
    nativeSha256=hashlib.sha256((root/'objectives-logic/libsm_native.so').read_bytes()).hexdigest()),indent=2)+'\n')
l.sm_shutdown();print('OBJECTIVE_LIVE_STATE_PASS',flush=True)
