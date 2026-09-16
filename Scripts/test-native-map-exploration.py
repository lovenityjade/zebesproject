#!/usr/bin/env python3
"""Source-derived map ownership, natural marks, relocated portals and SRAM.

Isolated gaming-pc native fixtures; no local runtime or manual traversal claim.
"""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-area-connections.py').read_text().split('cases=[];visited=set()')[0]
source=source.replace("root/'areas/native'","root/'map-exploration/native'").replace('areas/libsm_native.so','map-exploration/libsm_native.so')
exec(compile(source,'map-exploration-fixture','exec'))
audit=json.loads((root/'map-exploration/audit.json').read_text())
mark=routine('sm_map_exploration_mark',None,C.c_uint,C.c_uint)
portal=routine('sm_map_exploration_portal',None,C.c_int,C.c_int)
pack=routine('PackMapToSave',None);unpack=routine('UnpackMapFromSave',None)
pack_extra=routine('sm_map_exploration_pack',None);unpack_extra=routine('sm_map_exploration_unpack',None)
save=routine('SaveToSram',None,C.c_uint16);load=routine('LoadFromSram',C.c_uint8,C.c_uint16)
mirror=routine('LoadMirrorOfExploredMapTiles',None)
l.sm_map_exploration_value.argtypes=[C.c_int,C.c_int]
value=l.sm_map_exploration_value
map_data=routine('sm_seed_map_data',C.c_void_p,C.c_uint)
results=[]
def clear(area=0):
    C.memmove(ram,baseline,len(baseline));put(0x998,8);put(0x79f,area)
    C.memset(ram+0x7f7,0,256);C.memset(ram+0xcd52,0,2048)
def commit(mapping):
    assert configure(0,mapping);area_activate(0)
    assert select(items,100,m['sha256'].encode())
def counts():return [value(r,0) for r in range(12)]
def expected_counts(layout):
    want=[0]*12
    for p in audit['tiles']:
        region=p['owners'][layout]
        if region in [0,11]:continue
        address=(0x7f7 if p['area']==word(0x79f) else 0xcd52+256*p['area'])+p['byte']
        want[region]+=bool(read(address,1)[0]&p['mask'])
    return want
for layout,mapping in enumerate([[],list(range(32))]):
    commit(mapping);clear()
    totals=[audit['layouts']['area_rando' if layout else 'vanilla_layout'][r] if i not in [0,11] else 0 for i,r in enumerate(audit['regions'])]
    assert [value(r,1) for r in range(12)]==totals and value(-1,1)==sum(totals)
    assert counts()==[0]*12 and value(-1,0)==0
    for r in audit['rooms']:
        put(0x79b,r['room']);assert routine('sm_varia_region',C.c_int)()==r['owners'][layout],(layout,r)
    # Every source tile individually, then fully explored. Knowledge alone is never exploration.
    for p in audit['tiles']:
        clear(p['area']);mark(p['byte'],p['mask'])
        assert counts()==expected_counts(layout),(layout,p,counts())
        before=read(0,131072);mark(p['byte'],p['mask']);assert read(0,131072)==before
    C.memset(ram+0x7f7,255,256);C.memset(ram+0xcd52,255,2048)
    assert counts()==totals and value(-1,0)==sum(totals)
    # Current mirror supersedes stale saved bytes, as in gameplay before saving.
    clear(1);C.memset(ram+0xcd52+256,255,256);assert value(-1,0)==0
    results.append(dict(kind='ownership',layout=layout,tiles=len(audit['tiles']),rooms=len(audit['rooms']),totals=totals))
# Prove the generated native minimap routine reaches the new mark hook.
update=routine('UpdateMinimap',None)
for p in audit['slopes']:
    clear(p['area']);put(0x7a1,p['x']);put(0x7a3,p['y']-1);put(0xaf6,0);put(0xafa,0);put(0x763,0)
    update();assert read(0x7f7+p['byte'],1)[0]&p['mask'] and read(0x7f7+p['byte']-4,1)[0]&p['mask'],p
    assert counts()==expected_counts(1)
results.append(dict(kind='native_minimap_slopes',cases=8))
# Exact writeExploreMapAsm behavior, including same-physical-area mirror reloads.
for shift in range(32):
    mapping=[(shift-i)%32 for i in range(32)];commit(mapping)
    for i,j in enumerate(mapping):
        clear(audit['portals'][j]['roomArea'])
        expected=bytearray(read(0,131072))
        for src,dst in [(i,j)]+([] if i==j else [(j,i)]):
            p=audit['portals'][src]
            if not p['relocated']:continue
            expected[0xcd52+256*p['area']+p['byte']]|=p['mask']
            if p['area']==audit['portals'][dst]['roomArea']:
                a=word(0x79f);expected[0x7f7:0x8f7]=expected[0xcd52+256*a:0xcd52+256*(a+1)]
        portal(i,j)
        assert read(0,131072)==expected,(i,j)
        assert counts()==expected_counts(1)
        before=read(0,131072);portal(i,j);assert read(0,131072)==before
results.append(dict(kind='source_portal_semantics',cases=1024))
# Native map/save original bytes and the extra portal survive real checksummed SRAM.
commit(list(range(32)));clear(1)
C.memset(ram+0x7f7,255,256);C.memset(ram+0xcd52,255,2048)
expected=counts();save(0)
assert read(0xd91c+0x150,5)==b'ZME1\x80'
C.memset(ram+0x7f7,0,256);C.memset(ram+0xcd52,0,2048);C.memset(ram+0xd91c,0,1280)
assert load(0)==0;mirror()
assert counts()==expected,(counts(),expected)
assert read(0xcd52+256+201,1)[0]&128
# A historical save without our signature does not acquire an explored portal.
clear(1);C.memset(ram+0xd91c+0x150,0,5);unpack_extra();assert not read(0xcd52+256+201,1)[0]&128
# Original packed bytes remain identical and extension is outside the original range.
C.memset(ram+0xcd52,0xa5,2048);pack();before=read(0xd91c,1280);pack_extra()
assert read(0xd91c,0x150)==before[:0x150] and read(0xd91c+0x155,1280-0x155)==before[0x155:]
results.append(dict(kind='native_sram_reload',counts=expected,extension='ZME1',historical=True))
bank=[]
for slot in range(3):
    clear(1)
    for p in [p for p in audit['tiles'] if p['area']==1][:slot+1]:mark(p['byte'],p['mask'])
    if slot==1:((C.c_uint8*256).from_address(ram+0x7f7))[201]|=128
    bank.append(counts());save(slot)
for slot in [2,0,1,2,1,0]:
    C.memset(ram+0x7f7,0,256);C.memset(ram+0xcd52,0,2048)
    assert load(slot)==0;mirror()
    assert counts()==bank[slot] and bool(read(0x7f7+201,1)[0]&128)==(slot==1)
results.append(dict(kind='independent_sram_slots',counts=bank,loads=[2,0,1,2,1,0]))

# Pending/rejected world selection cannot switch counters or original map assets.
commit(list(range(32)));clear();assert value(-1,1)==1165
assert configure(1,[]);area_activate(1)
bad=(Item*100)(*items);bad[0].address=0
assert not select(bad,100,m['sha256'].encode()) and value(-1,1)==1165
assert C.string_at(romptr+0x1a820e,2)==b'\x25\xcc'
assert C.string_at(map_data(1),256)[32]&1  # x7,y8 byte32 mask1
assert select(None,0,None)
assert value(-1,0)==-1 and value(-1,1)==-1
assert C.string_at(romptr+0x1a820e,2)==original_runtime[0x1a820e:0x1a8210]
l.sm_tracker_configure(1,1,1);assert not C.string_at(map_data(1),256)[32]&1
for p in audit['slopes']:
    clear(p['area']);mark(p['byte'],p['mask']);assert not read(0x7f7+p['byte']-4,1)[0]&p['mask']
before=read(0,131072);portal(0,0);pack_extra();unpack_extra();assert before==read(0,131072)
assert C.string_at(romptr,0x300000)==original_runtime
assert l.sm_cpu_opcodes()==0
l.sm_shutdown();assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(root/'map-exploration/native-results.json').write_text(json.dumps(dict(pass_=True,results=results,cpuOpcodes=0),indent=2)+'\n')
print('MAP_EXPLORATION_NATIVE PASS',len(results),'groups')
