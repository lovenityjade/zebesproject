#!/usr/bin/env python3
"""Every area descriptor/setup, dependency transaction and added room PLM.

Controlled native fixtures on gaming-pc. Does not claim seed generation,
portal rendering or physical traversal of every connection.
"""
import os
import sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-tweaks.py').read_text().split('for slot,names in enumerate')[0]
source=source.replace("(root/'tweaks').mkdir(exist_ok=True)","(root/'areas').mkdir(exist_ok=True)").replace("root/'tweaks/native'","root/'areas/native'").replace('world-data/libsm_native.so','areas/libsm_native.so')
exec(compile(source,'area-fixture','exec'))
catalog=json.loads((root/'Randomizer/native_areas.json').read_text());aps=catalog['accessPoints']
# Rehydrate test-only expected bytes from pinned upstream, never public catalogs.
sys.path.insert(0,str(root/'Randomizer/upstream'))
from logic.logic import Logic
from rom.flavor import RomFlavor
from rom.ips import IPS_Patch
Logic.factory('vanilla');RomFlavor.factory(str(root/'Randomizer/upstream'))
access=RomFlavor.patchAccess
for patch in catalog['patches']:
    raw=access.getDictPatches().get(patch['name'])
    if raw is None:raw=IPS_Patch.load(access.getPatchPath(patch['name'])).toDict()
    flat={a+i:v for a,data in raw.items() for i,v in enumerate(data)}
    for span in patch['spans']:
        if 'data' not in span:
            span['data']=[flat[span['address']+i] for i in range(span['size'])]
            assert hashlib.sha256(bytes(span['data'])).hexdigest()==span['sha256']

l.sm_areas_configure.argtypes=[C.c_int,C.POINTER(C.c_uint8),C.c_int,C.c_char_p]
area_activate=routine('sm_areas_activate',None,C.c_int)
setup=routine('RunDoorSetupCode',None)
spark=routine('Samus_EndSuperJump',C.c_uint8)
romptr=C.c_void_p.from_address(base+symbols['g_rom']).value
assert select(None,0,None);original_runtime=C.string_at(romptr,0x300000)
def configure(slot,mapping):
    values=(C.c_uint8*len(mapping))(*mapping)
    return l.sm_areas_configure(slot,values,len(values),catalog['sha256'].encode())
def expected_rom(mapping):
    expected=bytearray(original_runtime)
    for patch in catalog['patches']:
        for s in patch['spans']:
            a=s['address'];expected[a:a+len(s['data'])]=bytes(s['data'])
    for i,j in enumerate(mapping):
        c=catalog['connections'][i][j];room=aps[j]['room']['RoomPtr'];door=aps[i]['exit']['DoorPtr'];a=0x10000+door
        expected[a:a+12]=room.to_bytes(2,'little')+bytes([c['bitFlag'],c['direction'],*c['cap'],*c['screen']])+c['distance'].to_bytes(2,'little')+c['asm'].to_bytes(2,'little')
        if door==0x93ea:expected[0x70008+room]=2
        if room==0x93fe:expected[0x7b7bb:0x7b7bd]=door.to_bytes(2,'little')
        for a in aps[j]['room'].get('songs',[]):expected[0x70000+a:0x70002+a]=bytes([aps[j]['entry']['song'],5])
    for p in m['placements']:expected[p['address']:p['address']+2]=p['plm'].to_bytes(2,'little')
    return expected

cases=[];visited=set()
# Each involution is a reciprocal, total mapping. All 32 shifts cover every
# directed pair exactly once, including loops needed by later world modes.
for offset in range(32):
    mapping=[(offset-i)%32 for i in range(32)]
    assert configure(0,mapping);area_activate(0)
    expected=expected_rom(mapping)
    assert select(items,100,m['sha256'].encode())
    actual=C.string_at(romptr,0x300000)
    assert actual==expected,[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:20]
    wrong=(Item*100)(*items);wrong[0].address=0
    assert not select(wrong,100,m['sha256'].encode()) and C.string_at(romptr,0x300000)==expected
    assert not configure(0,[0]*32) and not configure(0,[1,0])
    assert not configure(-1,mapping) and not configure(4,mapping)
    invalid=list(mapping);invalid[0]=32;assert not configure(0,invalid)
    invalid=[1,2,0,*range(3,32)]  # A bijection, but not reciprocal.
    assert not configure(0,invalid)
    assert not l.sm_areas_configure(0,(C.c_uint8*32)(*mapping),32,b'0'*64)
    transitions=[]
    for i,j in enumerate(mapping):
        visited.add((i,j));C.memmove(ram,baseline,len(baseline));c=catalog['connections'][i][j]
        put(0x78d,aps[i]['exit']['DoorPtr']);put(0xaf6,111);put(0xafa,222);put(0x9c2,999)
        for a in [0xb2c,0xb2e,0xb42,0xb44,0xb46,0xb48,0xa96]:put(a,7)
        put(0xa6e,2);put(0xdd0,0);put(0x741,0xbeef);put(0xe1e,0x1234)
        # Call the same original scroll routine independently as the expected
        # result, then restore RAM before the actual native door hook.
        before_scroll=read(0,131072)
        if c['asm']:routine('CallDoorDefSetupCode',None,C.c_uint32)(0x8f0000|c['asm'])
        scroll_expected=read(0xcd20,50)
        C.memmove(ram,before_scroll,len(before_scroll))
        setup();assert word(0x18a8)==128 and word(0x741)==0xbeef
        assert read(0xcd20,50)==scroll_expected
        assert (word(0xaf6),word(0xafa))==((c['x'],c['y']) if c['incompatible'] else (111,222))
        if c['incompatible']:
            assert all(word(a)==0 for a in [0xb2c,0xb2e,0xb42,0xb44,0xb46,0xb48,0xa96,0xa6e])
            assert read(0xa1c,12)==bytes(12) and read(0xa2a,6)==b'\xff'*6
            assert read(0xb10,8)==read(0xaf6,8)
        assert spark()==int(c['incompatible']) and word(0x9c2)==999
        assert spark()==0 and word(0x741)==0xbeef
        assert word(0xe1e)==(0 if c['exitAsm']=='door_transition_boss_exit_fix' else 0x1234)
        transitions.append(dict(source=aps[i]['name'],destination=aps[j]['name'],incompatible=c['incompatible']))
    # Configuring/activating another slot is pending until a valid transaction.
    assert configure(1,[]);area_activate(1)
    assert C.string_at(romptr,0x300000)==expected
    assert not select(wrong,100,m['sha256'].encode())
    assert C.string_at(romptr,0x300000)==expected
    assert select(None,0,None) and C.string_at(romptr,0x300000)==original_runtime
    cases.append(dict(mapping=mapping,transitions=transitions))
    print('AREA_ROUTING_PASS',len(cases),flush=True)
assert len(visited)==1024

# Shared SRAM bank selectors: A/B/C and staging slot retain independent worlds.
for slot in range(4):assert configure(slot,[(slot-i)%32 for i in range(32)])
bank_cases=[]
for slot in [0,1,2,3,0,2,1]:
    assert activate(slot,1);assert select(items,100,m['sha256'].encode())
    assert C.string_at(romptr,0x300000)==expected_rom([(slot-i)%32 for i in range(32)])
    bank_cases.append(slot)
assert activate(0,0);assert select(items,100,m['sha256'].encode())
expected=bytearray(original_runtime)
for p in m['placements']:expected[p['address']:p['address']+2]=p['plm'].to_bytes(2,'little')
assert C.string_at(romptr,0x300000)==expected

# Two source-defined patch phases: Le Coude's blinking-door enemy byte must
# win over the optional moat layout patch even though the world has that data.
world=json.loads((root/'Randomizer/native_world_data.json').read_text())
moat=next(p for p in world['patches'] if p['name']=='moat.ips')
world_order=(C.c_uint8*1)(moat['id'])
assert l.sm_start_configure(0,0,world_order,1,world['sha256'].encode())
assert activate(0,1);assert select(items,100,m['sha256'].encode())
assert C.string_at(romptr+0x1085dd,1)==b'\0'
assert l.sm_start_configure(0,0,None,0,world['sha256'].encode())
assert activate(0,1);assert select(items,100,m['sha256'].encode())

room_hook=routine('sm_start_room',None);clear_plms=routine('ClearPLMs',None)
plm_cases=[]
for name,p in catalog['roomPlms'].items():
    C.memmove(ram,baseline,len(baseline));clear_plms()
    put(0x79b,p['room']);put(0x7bb,p.get('state',0))
    width=original_runtime[0x70004+p['room']]*16;put(0x7a5,width)
    C.memset(ram+0xd8b0,0,64)
    scratch=read(0x12,6);room_hook();room_hook();assert read(0x12,6)==scratch
    for entry in p['plm_bytes_list']:
        header=entry[0]|entry[1]<<8;block=2*(entry[3]*width+entry[2])
        indices=[i for i in range(40) if word(0x1c37+i*2)==header and word(0x1c87+i*2)==block]
        assert len(indices)==1,(name,indices)
    plm_cases.append(name)

# Shared Wrecked Ship extra PLM is deduplicated when boss+area both apply.
boss=json.loads((root/'Randomizer/native_connections.json').read_text())
bossids={a['name']:a['id'] for a in boss['accessPoints']};bossmap=[0]*8
for a,b in boss['vanillaPairs']:bossmap[bossids[a]]=bossids[b];bossmap[bossids[b]]=bossids[a]
l.sm_connections_configure.argtypes=[C.c_int,C.POINTER(C.c_uint8),C.c_int,C.c_char_p]
assert l.sm_connections_configure(0,(C.c_uint8*8)(*bossmap),8,boss['sha256'].encode())
assert activate(0,1);assert select(items,100,m['sha256'].encode())
for state,count in [(0xcb08,1),(0xcb22,0)]:
    C.memmove(ram,baseline,len(baseline));clear_plms();put(0x79b,0xcaf6);put(0x7bb,state);put(0x7a5,80)
    room_hook();room_hook()
    assert sum(word(0x1c37+i*2)==0xc842 for i in range(40))==count

# Full refill uses only the native dispatch for the Tourian elevator pointer.
refill_cases=[]
for randomized in [True,False,True]:
    assert select(items,100,m['sha256'].encode()) if randomized else select(None,0,None)
    C.memmove(ram,baseline,len(baseline));put(0x78d,0x9222)
    for current,maximum,value in [(0x9c2,0x9c4,899),(0x9d6,0x9d4,300),(0x9c6,0x9c8,175),(0x9ca,0x9cc,35),(0x9ce,0x9d0,30)]:
        put(current,1);put(maximum,value)
    setup()
    actual=[word(a) for a in [0x9c2,0x9d6,0x9c6,0x9ca,0x9ce]]
    assert actual==([899,300,175,35,30] if randomized else [1]*5),actual
    refill_cases.append(dict(randomized=randomized,values=actual))
assert select(None,0,None);assert C.string_at(romptr,0x300000)==original_runtime
assert l.sm_cpu_opcodes()==0;l.sm_shutdown();assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(out/'verification.json').write_text(json.dumps(dict(routing=cases,uniquePairs=len(visited),slotSelection=bank_cases,
    roomPlms=plm_cases,sharedPlmDeduplicated=True,blinkOverridesLayout=True,refillCases=refill_cases,
    cpu=0,sourceRomUnchanged=True,nativeSha256=hashlib.sha256(libpath.read_bytes()).hexdigest(),
    scope='Controlled native door setup, PLM insertion and full-ROM transactions; not area-seed gameplay or map presentation'),indent=2)+'\n')
print('AREA_NATIVE_PASS',len(visited),'directed pairs',len(plm_cases),'room PLMs',flush=True)
