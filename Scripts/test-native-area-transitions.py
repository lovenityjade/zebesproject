#!/usr/bin/env python3
"""Original door collision -> fade/scroll/load -> gameplay, isolated gaming-pc.

Source rooms and equipment are prepared fixtures. A real door block is selected
from the loaded level/BTS/list; the native collision routine initiates the
transition. No destination-room injection is used during that transition.
"""
import os
import sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-area-connections.py').read_text().split('cases=[];visited=set()')[0]
source=source.replace("root/'areas/native'","root/'areas/transitions'")
exec(compile(source,'area-transition-fixture','exec'))
ids={ap['name']:ap['id'] for ap in aps}
pairs=[
    ('Crocomire Room Top','West Ocean Left'),
    ('West Ocean Left','Crab Shaft Right'),
    ('Crab Shaft Right','Crocomire Speedway Bottom'),
    ('Crab Hole Bottom Left','Green Brinstar Elevator'),
    ('Green Brinstar Elevator','East Tunnel Top Right'),
    ('East Tunnel Top Right','Aqueduct Top Left'),
    ('Red Brinstar Elevator','Warehouse Entrance Right'),
    ('Main Street Bottom','Lower Mushrooms Left'),
    ('Red Fish Room Left','Keyhunter Room Bottom'),
    ('Morph Ball Room Left','Caterpillar Room Top Right'),
    ('Golden Four','Kronic Boost Room Bottom Left'),
    ('Three Muskateers Room Left','Moat Right'),
]
results=[]
for a,b in pairs:
    i,j=ids[a],ids[b];mapping=list(range(32));mapping[i],mapping[j]=j,i
    assert configure(0,mapping);area_activate(0);assert select(items,100,m['sha256'].encode())
    assert l.sm_test_all_equipment();assert l.sm_test_area_connection(j)
    for frame in range(2000):
        step()
        if l.sm_state()==8 and l.sm_room()==aps[i]['room']['RoomPtr']:break
    else:raise AssertionError(('Source fixture',a))
    wait(440);assert l.sm_state()==8 and routine('HasQueuedMusic',C.c_uint8)()==0
    width=word(0x7a5);height=word(0x7a7);doorlist=word(0x7b5);blocks=[]
    for block in range(width*height):
        if word(0x10002+2*block)>>12!=9:continue
        bts=read(0x16402+block,1)[0]&127
        ptr=int.from_bytes(C.string_at(romptr+0x70000+doorlist+2*bts,2),'little')
        if ptr==aps[i]['exit']['DoorPtr']:blocks.append(block)
    assert blocks,('Original door block missing',a,hex(aps[i]['exit']['DoorPtr']))
    block=blocks[len(blocks)//2];x=(block%width)*16+8;y=(block//width)*16+8
    # Place Samus at the selected block and invoke the same collision routine
    # that the movement engine invokes after reaching an open door.
    put(0xaf6,x);put(0xafa,y);put(0xb10,x);put(0xb14,y)
    put(0xdc4,block);put(0xe16,0)
    direction=aps[i]['exit']['direction']
    collide=routine('BlockColl_Vert_Door' if direction&2 else 'BlockColl_Horiz_Door',C.c_uint8,C.c_void_p)
    assert collide(None)==0 and l.sm_state()==9
    assert word(0x78d)==aps[i]['exit']['DoorPtr']
    states=[]
    for frame in range(2000):
        step()
        if not states or states[-1]!=l.sm_state():states.append(l.sm_state())
        if l.sm_state()==8:break
    else:raise AssertionError(('Door transition stalled',a,b,l.sm_state(),hex(word(0x993))))
    assert l.sm_room()==aps[j]['room']['RoomPtr'],(a,b,hex(l.sm_room()))
    assert 11 in states,states
    wait(180)
    assert l.sm_state()==8 and l.sm_room()==aps[j]['room']['RoomPtr']
    assert routine('HasQueuedMusic',C.c_uint8)()==0
    assert l.sm_cpu_opcodes()==0
    label='%02d-to-%02d'%(i,j);capture(label)
    results.append(dict(source=a,destination=b,block=block,states=states,transitionFrames=frame+1,room=l.sm_room(),capture=label+'.png'))
    print('AREA_TRANSITION_PASS',a,'->',b,frame+1,flush=True)
l.sm_shutdown();assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(out/'verification.json').write_text(json.dumps(dict(transitions=results,cpu=0,sourceRomUnchanged=True,
    nativeSha256=hashlib.sha256(libpath.read_bytes()).hexdigest(),
    scope='Original door collision and transition pipeline with prepared source rooms, equipment and reciprocal mappings; not generated-seed progression'),indent=2)+'\n')
