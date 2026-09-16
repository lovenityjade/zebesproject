#!/usr/bin/env python3
"""Source condition thresholds, native frame/gate hooks and SRAM on gaming-pc."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-area-connections.py').read_text().split('cases=[];visited=set()')[0]
source=source.replace("root/'areas/native'","root/'objectives/native'").replace('areas/libsm_native.so','objectives/libsm_native.so')
exec(compile(source,'objectives-fixture','exec'))
audit=json.loads((root/'objectives/catalog.json').read_text())
map_audit=json.loads((root/'objectives/map-audit.json').read_text())
event_audit=json.loads((root/'objectives/events-audit.json').read_text())
class Plan(C.Structure):
    _fields_=[('version',C.c_uint32),('size',C.c_uint32)]+[(n,C.c_uint16) for n in ['count','required','flags','item_mask','beam_mask']]+[(n,C.c_uint8*s) for n,s in [('goals',18),('item_counted',100),('area_counted',100)]]+[('enemy_totals',C.c_uint16*6),('map_totals',C.c_uint16*12)]
assert C.sizeof(Plan)==audit['abiSize']==272
config=l.sm_objectives_configure;config.argtypes=[C.c_int,C.POINTER(Plan),C.c_char_p]
state=l.sm_objectives_state;state.argtypes=[C.c_int]
value=l.sm_objectives_value;value.argtypes=[C.c_int,C.c_int]
class ObjectiveSnapshot(C.Structure):
    _fields_=[('version',C.c_uint32),('size',C.c_uint32),('state',C.c_int32*8),('goals',C.c_int32*18),('values',(C.c_int32*5)*18)]
assert C.sizeof(ObjectiveSnapshot)==472
l.sm_objectives_snapshot.argtypes=[C.POINTER(ObjectiveSnapshot),C.c_uint32]
def objective_snapshot():
    s=ObjectiveSnapshot();assert l.sm_objectives_snapshot(C.byref(s),C.sizeof(s))
    assert s.version==1 and s.size==C.sizeof(s)
    assert list(s.state)==[state(i) for i in range(8)]
    for i in range(s.state[0]):assert list(s.values[i])==[value(i,f) for f in range(5)]
    assert all(i==-1 for i in s.goals[s.state[0]:])
    assert all(list(row)==[-1]*5 for row in s.values[s.state[0]:])
    return s
frame=routine('sm_objectives_frame',None)
event=l.sm_objective_event;event.argtypes=[C.c_uint]
custom_mark=routine('sm_objective_event_mark',None,C.c_uint)
statue=routine('AnimtilesInstr_SetEventHappened',C.c_uint16,C.c_uint16,C.c_uint16)
save=routine('SaveToSram',None,C.c_uint16);load=routine('LoadFromSram',C.c_uint8,C.c_uint16)
byname={g['name']:g['id'] for g in audit['goals']}
regions=map_audit['regions'];results=[]
# Location graph ownership comes from the source Python logic, independently of
# the compiled condition table. Original ROM location bits are never index bits.
sys.path.insert(0,str(root/'Randomizer/upstream'))
from logic.logic import Logic
Logic.factory('vanilla')
locations={p.Address:p for p in Logic.locations() if not p.isBoss()}
item_regions=[regions.index(locations[p['address']].GraphArea) for p in geometry]

def mark(n):
    if n>=128:custom_mark(n)
    else:
        address=0xd820+(n>>3);C.memmove(ram+address,bytes([read(address,1)[0]|(1<<(n&7))]),1)
def plan(goals,required=None,layout=0,flags=0):
    p=Plan();p.version=1;p.size=C.sizeof(p);p.count=len(goals);p.required=required or p.count;p.flags=flags
    p.goals[:len(goals)]=goals;p.item_counted[:]=[1]*100;p.area_counted[:]=[1]*100
    p.item_mask=0xf32f;p.beam_mask=0x100f;p.enemy_totals[:]=[62,19,22,20,5,17]
    totals=map_audit['layouts']['area_rando' if layout else 'vanilla_layout']
    p.map_totals[:]=[totals[r] if i not in [0,11] else 0 for i,r in enumerate(regions)]
    return p
def clear():
    C.memmove(ram,baseline,len(baseline));put(0x998,8);put(0x79b,0x91f8);put(0x79f,0)
    C.memset(ram+0xd820,0,16);C.memset(ram+0xd870,0,64)
    C.memset(ram+0xd91c+0x180,0,37)
    C.memset(ram+0x7f7,0,256);C.memset(ram+0xcd52,0,2048)
    put(0x9a4,0);put(0x9a8,0);put(0x5f5,0);put(0x592,0)
    C.memset(ram+0x643,0,6);C.memset(ram+0x656,0,48)
def commit(p,slot=0,layout=0):
    assert configure(slot,list(range(32)) if layout else [])
    assert config(slot,C.byref(p),audit['sha256'].encode())
    assert activate(slot,1);assert select(items,100,m['sha256'].encode())
def collect(index):
    bit=int.from_bytes(C.string_at(romptr+geometry[index]['address']+4,2),'little')&255
    address=0xd870+(bit>>3);C.memmove(ram+address,bytes([read(address,1)[0]|(1<<(bit&7))]),1)
def explore(tile):
    a=(0x7f7 if tile['area']==word(0x79f) else 0xcd52+256*tile['area'])+tile['byte']
    C.memmove(ram+a,bytes([read(a,1)[0]|tile['mask']]),1)

for goal in audit['goals']:
    if not goal['supported']:continue
    p=plan([goal['id']]);commit(p);clear();kind=goal['kind'];arg=goal['arg']
    assert value(0,0)==0
    assert value(0,4)==int(kind=='nothing'),goal
    if kind=='event':mark(arg)
    elif kind in ['bosses','minibosses']:
        ids=[72,88,96,80] if kind=='bosses' else [73,97,81,82]
        for i in range(arg):
            assert value(0,4)==0;mark(ids[i]);assert value(0,1)==i+1
    elif kind=='item_percent':
        for i in range(arg-1):collect(i)
        assert value(0,4)==0 and value(0,2)==arg
        collect(arg-1)
    elif kind=='upgrades':
        put(0x9a4,0xf32f);put(0x9a8,0x100e);assert not value(0,4)
        put(0x9a8,0x100f);assert value(0,4)
        put(0x9a4,0xf36f);assert not value(0,4) # source equality, not subset
        put(0x9a4,0xf32f)
    elif kind=='area_clear':
        indexes=[i for i,r in enumerate(item_regions) if r==arg]
        for i in indexes:collect(i)
        assert value(0,1)==value(0,2)==len(indexes) and not value(0,4)
        room=next(r['room'] for r in map_audit['rooms'] if r['owners'][0]==arg)
        put(0x79b,room);frame();assert event(147+arg) and event(132+arg)
    elif kind in ['map_percent','area_explore']:
        tiles=[t for t in map_audit['tiles'] if t['owners'][0] not in [0,11] and (kind=='map_percent' or t['owners'][0]==arg)]
        needed=(len(tiles)*arg+99)//100 if kind=='map_percent' else len(tiles)
        assert value(0,2)==needed
        for t in tiles[:needed-1]:explore(t)
        assert not value(0,4)
        explore(tiles[needed-1])
    elif kind=='robots':
        for e in [82,66,145]:mark(e)
        assert value(0,1)==3 and not value(0,4);mark(12)
    elif kind=='animals':
        mark(158);mark(159);assert value(0,1)==2 and not value(0,4);put(0x79f,1)
    elif kind=='enemy_family':
        for e in event_audit['enemies']:
            if e['type']==arg:mark(e['event'])
        assert value(0,1)==value(0,2)==p.enemy_totals[arg]
        assert not value(0,4);mark(198+arg)
    elif kind!='nothing':raise AssertionError(kind)
    assert value(0,4)==1,goal
    before=read(0,131072);values=[value(0,i) for i in range(5)]
    snapshot=objective_snapshot();assert snapshot.goals[0]==goal['id'] and list(snapshot.values[0])==values
    assert before==read(0,131072),'Read-only progress mutated RAM'
    frame();assert value(0,0)==state(4)==state(5)==1,goal
    assert objective_snapshot().values[0][0]==1
    results.append(dict(id=goal['id'],name=goal['name'],values=values))

# Exercise the maximum 18-slot descriptor and final completion-event index.
p=plan([g['id'] for g in audit['goals'] if g['supported']][:18]);commit(p);clear()
for i in range(18):mark(162+2*i)
for _ in range(18):frame()
assert state(0)==state(2)==18 and state(4)==state(5)==1
# Fractional thresholds and independent split-count vs actual-item membership.
p=plan([byname['collect 25% items']]);p.item_counted[:]=[1]*7+[0]*93
commit(p);clear();collect(90);assert value(0,1)==0 and value(0,2)==2
collect(0);assert not value(0,4);collect(1);assert value(0,4)
# Once-per-frame checks, one SFX at cycle end, mute after required met.
goals=[g['id'] for g in audit['goals'] if g['kind']=='event' and g['supported']][:12]
p=plan(goals,required=2,flags=3);commit(p);clear()
for g in goals:mark(audit['goals'][g]['arg'])
put(0x998,15);frame();assert state(2)==0;put(0x998,8)
for i in range(len(goals)-1):
    frame();assert state(2)==i+1 and state(4)==0 and read(0x647,1)==b'\x00'
frame();assert state(2)==len(goals) and state(4)==state(5)==1
assert read(0x647,1)==b'\x01' and read(0x666,1)==b'\x19'
assert not state(7);put(0x79b,0xa66a);frame();assert state(7)
# The public frame entry calls sm_seed_frame twice; goal evaluation must still
# advance exactly once per real native frame, not just once per helper call.
p=plan([byname['kill kraid'],byname['kill phantoon'],byname['kill ridley']]);commit(p);clear()
for e in [72,88,80]:mark(e)
for expected in [1,2,3]:
    step();assert state(2)==expected,(expected,state(2))
# Continue collecting optional goals silently after the required quota.
p=plan([byname['kill kraid'],byname['kill ridley']],required=1,flags=1);commit(p);clear();mark(72)
frame();frame();assert state(4)==1 and state(5)==0 and state(3)==0
mark(80);frame();frame();assert state(5)==1 and read(0x647,1)==b'\x01'
# Regions with no items cannot alias the animals/scavenger event identities.
for region in [0,11]:
    commit(p);clear();put(0x79b,next(r['room'] for r in map_audit['rooms'] if r['owners'][0]==region))
    frame();assert not event(158) and not event(147)
# Vanilla statue instructions advance identically; configured goals gate only four operands.
operands=[0x8402,0x846a,0x84d2,0x853a]
for j in operands:
    e=int.from_bytes(C.string_at(romptr+0x38000+(j&0x7fff),2),'little')
    commit(p);clear();assert statue(0,j)==j+2 and not event(e)
    mark(10);assert statue(0,j)==j+2 and event(e)
    assert config(0,None,audit['sha256'].encode());assert activate(0,1);assert select(items,100,m['sha256'].encode())
    clear();assert statue(0,j)==j+2 and event(e)
# Failed configuration/seed activation must preserve committed behavior.
p=plan([byname['kill kraid']]);commit(p);clear()
for field,bad in [('version',2),('size',0),('count',19),('required',0),('flags',8),('item_mask',0xffff),('beam_mask',0xffff)]:
    q=Plan.from_buffer_copy(p);setattr(q,field,bad);assert not config(0,C.byref(q),audit['sha256'].encode())
q=Plan.from_buffer_copy(p);q.goals[0]=byname['finish scavenger hunt'];assert not config(0,C.byref(q),audit['sha256'].encode())
q=Plan.from_buffer_copy(p);q.count=2;q.goals[1]=q.goals[0];assert not config(0,C.byref(q),audit['sha256'].encode())
for field in ['item_counted','area_counted','enemy_totals','map_totals']:
    q=Plan.from_buffer_copy(p);getattr(q,field)[0]=999 if field.endswith('totals') else 2
    assert not config(0,C.byref(q),audit['sha256'].encode())
assert not config(-1,C.byref(p),audit['sha256'].encode()) and not config(4,C.byref(p),audit['sha256'].encode())
assert not config(0,C.byref(p),b'0'*64)
q=plan([byname['kill ridley']]);assert config(1,C.byref(q),audit['sha256'].encode());assert activate(1,1)
bad=(Item*100)(*items);bad[0].address=0;assert not select(bad,100,m['sha256'].encode())
mark(72);assert value(0,4) # applied Kraid, not pending Ridley
assert configure(1,list(range(32)));assert not activate(1,1) # stale objective map totals
# Independent objective lists + real checksummed A/B/C save/reload.
patterns=[]
for slot,names in enumerate([['kill kraid'],['kill ridley','kill phantoon'],['kill draygon','kill kraid','kill ridley']]):
    p=plan([byname[n] for n in names],required=1);commit(p,slot);clear()
    mark(audit['goals'][p.goals[0]]['arg'])
    for _ in names:frame()
    assert state(2)==state(4)==1
    save(slot);patterns.append((p,read(0xd91c+0x180,37)))
for slot in [2,0,1,2,1,0]:
    assert activate(slot,1);assert select(items,100,m['sha256'].encode())
    clear();assert load(slot)==0
    assert state(0)==patterns[slot][0].count and state(2)==state(4)==1
    assert read(0xd91c+0x180,37)==patterns[slot][1]
# Vanilla clears the effective contract and never fabricates objective events.
assert activate(0,0);assert select(None,0,None);clear();before=read(0,131072);frame()
assert state(0)==0 and value(0,0)==-1 and read(0,131072)==before
assert l.sm_cpu_opcodes()==0
assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
report=dict(conditionCases=results,scheduler=True,gateOperands=operands,invalidPlans=True,pendingRollback=True,sramSlots=3,reloads=6,vanilla=True,romSha256=rom_hash,nativeSha256=hashlib.sha256((root/'objectives/libsm_native.so').read_bytes()).hexdigest())
(root/'objectives/conditions-results.json').write_text(json.dumps(report,indent=2)+'\n')
print('OBJECTIVES_PASS',len(results),'conditions; frame scheduling, native gates, invalid plans, SRAM A/B/C',flush=True)
