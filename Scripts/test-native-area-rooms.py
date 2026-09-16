#!/usr/bin/env python3
"""Native destination-room load/frames for each area endpoint and source axis.

Uses controlled source mappings, not generated seeds or manual traversal.
The test hook loads the real room using its actual configured incoming door.
"""
import os
import sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-area-connections.py').read_text().split('cases=[];visited=set()')[0]
source=source.replace("root/'areas/native'","root/'areas/render'")
exec(compile(source,'area-room-fixture','exec'))
source_directions={a['exit']['direction'] for a in aps}
sources=[next(a['id'] for a in aps if a['exit']['direction']==direction) for direction in sorted(source_directions)]
results=[]
for j,destination in enumerate(aps):
    for i in sources:
        mapping=list(range(32));mapping[i],mapping[j]=j,i
        assert configure(0,mapping);area_activate(0)
        assert select(items,100,m['sha256'].encode())
        assert l.sm_test_all_equipment()
        assert l.sm_test_area_connection(i)
        for frame in range(2000):
            step()
            if l.sm_state()==8 and l.sm_room()==destination['room']['RoomPtr']:break
        else:raise AssertionError(('Area room load',aps[i]['name'],destination['name'],l.sm_state(),hex(l.sm_room())))
        # This fixture re-enters the save-load pipeline, including Samus's
        # 360-frame arrival fanfare. Do not start another synthetic reload
        # while its delayed music commands still belong to the preceding room.
        wait(440)
        assert routine('HasQueuedMusic',C.c_uint8)()==0
        assert l.sm_state()==8 and l.sm_room()==destination['room']['RoomPtr'],(i,j,l.sm_state(),hex(l.sm_room()))
        # The header chosen by the native loader contains the actual destination
        # and its dimensions; this covers both compressed-data area patches.
        assert word(0x78d)==aps[i]['exit']['DoorPtr']
        assert word(0x7a5)==original_runtime[0x70004+destination['room']['RoomPtr']]*16
        assert l.sm_cpu_opcodes()==0
        label='%02d-from-%d'%(j,aps[i]['exit']['direction'])
        capture(label)
        results.append(dict(source=aps[i]['name'],destination=destination['name'],room=l.sm_room(),framesToGameplay=frame+1,
                            incompatible=catalog['connections'][i][j]['incompatible'],capture=label+'.png'))
        print('AREA_ROOM_PASS',len(results),aps[i]['name'],'->',destination['name'],flush=True)
l.sm_shutdown();assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(out/'verification.json').write_text(json.dumps(dict(rooms=results,cpu=0,sourceRomUnchanged=True,
    nativeSha256=hashlib.sha256(libpath.read_bytes()).hexdigest(),
    scope='Real native destination-room loading/decompression and 440 gameplay frames (including save-load fanfare) for each area AP and source direction; controlled reciprocal mappings, not seeded traversal'),indent=2)+'\n')
