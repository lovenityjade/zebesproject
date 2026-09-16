#!/usr/bin/env python3
"""Scavenger native PLM gates, Ridley, original SRAM slots and HUD browsing."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-objectives.py').read_text().split("for goal in audit['goals']:")[0]
source=source.replace('objectives/libsm_native.so','scavenger/libsm_native.so').replace("root/'objectives/native'","root/'scavenger/native'")
exec(compile(source,'scavenger-native-fixture','exec'))
scav=json.loads((root/'Randomizer/native_scavenger.json').read_text())
class Hunt(C.Structure):
    _fields_=[('version',C.c_uint32),('size',C.c_uint32),('count',C.c_uint16),('order',C.c_uint16*17)]
assert C.sizeof(Hunt)==scav['abiSize']==44
configure_hunt=l.sm_scavenger_configure;configure_hunt.argtypes=[C.c_int,C.POINTER(Hunt),C.c_char_p]
st=l.sm_scavenger_state;st.argtypes=[C.c_int]
allows=routine('sm_scavenger_allows',C.c_int,C.c_uint)
pickup=routine('PlmInstr_SetItemBit',C.c_void_p,C.c_void_p,C.c_uint16)
tick=routine('sm_scavenger_frame',None,C.c_int)
appear=routine('CeresRidley_A377',None)
die=routine('sm_scavenger_ridley_dead',None)
new_game=routine('sm_scavenger_new_game',None)
label=routine('sm_scavenger_label',C.c_char_p)
byhud={p['hud']:p for p in scav['locations']}
def hunt(order):
    h=Hunt();h.version=1;h.size=C.sizeof(h);h.count=len(order)
    h.order[:len(order)]=[byhud[i]['id']<<8|i for i in order];return h
def apply(order,slot=0):
    h=hunt(order);assert configure_hunt(slot,C.byref(h),scav['sha256'].encode())
    commit(plan([byname['kill kraid']]),slot);clear();new_game();return h
def pick(location):
    pointer=C.create_string_buffer(16);p=C.addressof(pointer)+6
    put(0x1dc7,location)
    before=read(0,131072);result=pickup(p,0)
    if result==p-6:assert before==read(0,131072),'Denied pickup changed RAM'
    else:assert result==p
    return result==p

cases=0
# Every authored location occupies each possible position in a full order.
for shift in range(17):
    order=list(range(shift,17))+list(range(shift));apply(order)
    for pos,hud in enumerate(order):
        assert st(1)==pos and st(2)==byhud[hud]['id']
        for future in order[pos+1:]:
            assert not allows(byhud[future]['id']) and not pick(byhud[future]['id']);cases+=1
        assert pick(0xfe) and st(1)==pos # unrelated location stays collectible
        assert pick(byhud[hud]['id']) and st(1)==pos+1
        assert pick(byhud[hud]['id']) and st(1)==pos+1 # no duplicate advancement
        assert event(147) and bool(event(129))==(pos==16)
        cases+=1
    assert st(3) and label().decode()==scav['labels'][-1]

# Seven original data bytes keep Ridley's door usable while he is deferred.
# Reverting a slot restores the complete non-Scavenger world byte-for-byte.
apply([0,1]);without_ridley=C.string_at(romptr,0x300000)
for order in ([0,16],[16,0],[0,1]):
    apply(order);expected=bytearray(without_ridley)
    if 16 in order:
        expected[0x78e98:0x78e9e]=bytes([0x42,0xc8,0x0e,0x06,0x5a,0x8c])
        expected[0x10a638]=0
    assert C.string_at(romptr,0x300000)==expected

# Strict catalog/word validation is transactional.
h=apply([0,1,16]);before=st(0)
for field,bad in [('version',2),('size',0),('count',0),('count',18)]:
    wrong=Hunt.from_buffer_copy(h);setattr(wrong,field,bad)
    assert not configure_hunt(0,C.byref(wrong),scav['sha256'].encode()) and st(0)==before
for index,value in [(0,0xffff),(0,0xff11),(0,0xaa00),(1,h.order[0]),(3,h.order[0] or 1)]:
    wrong=Hunt.from_buffer_copy(h);wrong.order[index]=value
    assert not configure_hunt(0,C.byref(wrong),scav['sha256'].encode())
assert not configure_hunt(4,C.byref(h),scav['sha256'].encode())
assert not configure_hunt(0,C.byref(h),b'wrong')
put(0xd8f4,0xffff);assert st(1)==-1 and not st(3)
assert all(not allows(byhud[x]['id']) for x in [0,1,16])

# Appearance is held at its original timer seam until Ridley is eligible.
apply([0,16]);put(0x79f,2);put(0xfb2,1);put(0xfa8,0xa377)
appear();assert word(0xfb2)==1 and word(0xfa8)==0xa377
assert pick(byhud[0]['id']);appear()
assert word(0xfb2)==0 and word(0xfa8)==0xa389
# Execute the real native death-state transition, not only its helper.
# Keep projectile/enemy allocation occupied to isolate the timer/event seam.
for slot in range(32):put(0xf78+slot*64,1)
put(0x800e,5);put(0x7836,0);put(0xfb2,1)
death_transition=routine('Ridley_Func_64',None)
death_transition();assert st(1)==1 and not event(80)
death_transition();assert st(3) and event(80) and st(1)==2 and word(0xfa8)==0xc5a8
die();assert st(1)==2
# Ceres is never gated by the Norfair objective.
apply([0,16]);put(0x79f,6);put(0xfb2,0);put(0xfa8,0xa377)
appear();assert word(0xfa8)==0xa389 and st(1)==0

# Real native checksummed A/B/C saves, including partially complete and done.
patterns=[]
for slot,order in enumerate([[0,1,16],[1,0,16],[16,0,1]]):
    apply(order,slot)
    for hud in order[:slot+1]:assert pick(byhud[hud]['id'])
    save(slot);patterns.append((order,read(0xd8f4,2),read(0xd91c+0x180,37)))
for slot in [2,0,1,2,1,0]:
    assert activate(slot,1) and select(items,100,m['sha256'].encode());clear();assert load(slot)==0
    assert st(1)==slot+1 and st(0)==3
    assert read(0xd8f4,2)==patterns[slot][1] and read(0xd91c+0x180,37)==patterns[slot][2]
# A pending slot/invalid seed must not publish a different order.
before=[l.sm_scavenger_entry(i) for i in range(3)]
assert activate(1,1);assert [l.sm_scavenger_entry(i) for i in range(3)]==before
bad=(Item*100)(*items);bad[0].address=0
assert not select(bad,100,m['sha256'].encode()) and [l.sm_scavenger_entry(i) for i in range(3)]==before

# Native source X/Y browsing: first action reveals current, later actions
# move within the remaining route. Saved progress never follows the cursor.
apply([0,1,16]);assert pick(byhud[0]['id']);put(0x998,15);put(0x8f,0);tick(1)
assert st(4) and label()==b'Press X-Y ' and st(1)==1
def key(mask):
    put(0x8f,mask);tick(1);put(0x8f,0);tick(1)
key(0x40);assert not st(4) and st(5)==2
key(0x40);assert st(5)==3
key(0x40);assert st(5)==3
key(0x4000);assert st(5)==2
key(0x4000);assert st(5)==2 and st(1)==1
save(0);key(0x40);assert st(5)==3 and st(1)==1
put(0x998,16);tick(1);assert st(5)==2 and not st(4)
clear();assert load(0)==0 and st(1)==1
put(0x998,15);tick(0);assert not st(4)

# Disabling the applied contract restores original pickup behavior, even if
# the original save word contains unrelated historical bytes.
assert activate(0,0) and select(None,0,None);clear();put(0xd8f4,0xbabe)
before=read(0xd91c+0x180,37);assert pick(byhud[0]['id']) and word(0xd8f4)==0xbabe
assert not st(0) and label() is None and read(0xd91c+0x180,37)==before
assert l.sm_cpu_opcodes()==0
(root/'scavenger/native-results.json').write_text(json.dumps(dict(passed=True,pickupCases=cases,
    authoredLocations=17,ridley=True,ceresPreserved=True,ridleyDoorDataAndRestoration=True,invalidPlans=True,corruptIndexFailsClosed=True,
    nativeSramSlots=3,reloads=6,pendingRollback=True,pauseBrowsing=True,vanilla=True,cpuOpcodes=0,
    nativeSha256=hashlib.sha256((root/'scavenger/libsm_native.so').read_bytes()).hexdigest(),scope=__doc__),indent=2)+'\n')
l.sm_shutdown();print('SCAVENGER_NATIVE_PASS',cases,flush=True)
