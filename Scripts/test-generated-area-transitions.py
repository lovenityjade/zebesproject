#!/usr/bin/env python3
"""Real native door transitions using five generated area worlds.

The source-room load and inventory are controlled fixtures. Collision then runs
the original transition pipeline without injecting the destination. This does
not claim manual traversal or reachability with the seed's starting inventory.
"""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-area-connections.py').read_text().split('cases=[];visited=set()')[0]
source=source.replace("root/'areas/native'","root/'areas/generated-transitions'")
exec(compile(source,'generated-area-fixture','exec'))
lib=l
class Indicator(C.Structure):
    _fields_=[('location',C.c_uint16),('plm',C.c_uint16)]
source=(root/'test-native-start-slots.py').read_text().split('lib.sm_start_configure.argtypes=')[1].split('def register(i):')[0]
exec(compile('lib.sm_start_configure.argtypes='+source,'generated-world-config','exec'))
ids={a['name']:a['id'] for a in aps}
colors=json.loads((root/'Randomizer/native_door_colors.json').read_text())
forced_blue=['GreenPiratesShaftBottomRight','KihunterBottom','GreenHillZoneTopRight','NoobBridgeRight',
    'KronicBoostBottomLeft','CrocomireSpeedwayBottom','CrabShaftRight','LeCoudeBottom','RedBrinstarElevatorTop']
results=[]
for seed in range(5):
    m=json.loads((root/'area-seeds'/f'seed-{seed:02d}.json').read_text())['manifest']
    plans=[m];start_config(0,0);assert activate(0,1)
    items=(Item*100)(*[Item(p['address'],p['plm'],p.get('kind',0)) for p in m['placements']])
    assert select(items,100,m['sha256'].encode())
    mapping=m['nativeContext']['topology']['nativeAreas']['destinations']
    if seed==2:
        byname={loc['name']:color for loc,color in zip(colors['locations'],m['nativeContext']['doorColors']['colors'])}
        assert all(byname[name]==0 for name in forced_blue),byname
    for name in ['Crocomire Room Top','West Ocean Left','Crab Shaft Right']:
        i=ids[name];j=mapping[i]
        assert l.sm_test_all_equipment() and l.sm_test_area_connection(j)
        for frame in range(2000):
            step()
            if l.sm_state()==8 and l.sm_room()==aps[i]['room']['RoomPtr']:break
        else:raise AssertionError(('Source load',seed,name))
        wait(440);assert l.sm_state()==8 and routine('HasQueuedMusic',C.c_uint8)()==0
        width=word(0x7a5);height=word(0x7a7);doorlist=word(0x7b5);blocks=[]
        for block in range(width*height):
            if word(0x10002+2*block)>>12!=9:continue
            bts=read(0x16402+block,1)[0]&127
            ptr=int.from_bytes(C.string_at(romptr+0x70000+doorlist+2*bts,2),'little')
            if ptr==aps[i]['exit']['DoorPtr']:blocks.append(block)
        assert blocks,('Missing source door',seed,name)
        block=blocks[len(blocks)//2];x=block%width*16+8;y=block//width*16+8
        put(0xaf6,x);put(0xafa,y);put(0xb10,x);put(0xb14,y);put(0xdc4,block);put(0xe16,0)
        collide=routine('BlockColl_Vert_Door' if aps[i]['exit']['direction']&2 else 'BlockColl_Horiz_Door',C.c_uint8,C.c_void_p)
        assert collide(None)==0 and l.sm_state()==9
        assert word(0x78d)==aps[i]['exit']['DoorPtr']
        states=[]
        for frame in range(2000):
            step()
            if not states or states[-1]!=l.sm_state():states.append(l.sm_state())
            if l.sm_state()==8:break
        else:raise AssertionError(('Stalled transition',seed,name,l.sm_state()))
        assert l.sm_room()==aps[j]['room']['RoomPtr'] and 11 in states
        wait(180)
        assert l.sm_state()==8 and l.sm_room()==aps[j]['room']['RoomPtr']
        assert routine('HasQueuedMusic',C.c_uint8)()==0 and l.sm_cpu_opcodes()==0
        label=f'{seed:02d}-{i:02d}-to-{j:02d}';capture(label)
        results.append(dict(seed=m['seed'],fingerprint=m['sha256'],source=name,destination=aps[j]['name'],
            states=states,transitionFrames=frame+1,room=l.sm_room(),capture=label+'.png'))
        print('GENERATED_AREA_TRANSITION_PASS',m['seed'],name,'->',aps[j]['name'],flush=True)
l.sm_shutdown()
assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(out/'verification.json').write_text(json.dumps(dict(passed=True,transitions=results,forcedBlueAreaDoors=forced_blue,
    nativeSha256=hashlib.sha256(libpath.read_bytes()).hexdigest(),cpu=0,sourceRomUnchanged=True,scope=__doc__),indent=2)+'\n')
