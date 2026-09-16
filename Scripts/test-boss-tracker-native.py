#!/usr/bin/env python3
"""Gaming-pc isolated real-core marker rendering/publication regression."""
import sys
from pathlib import Path
root=Path(sys.argv[1])
s=(root/'test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
s=s.replace("out=root/'tracker-results'","out=root/'boss-tracker/SMTests'").replace('out.mkdir(exist_ok=True)','out.mkdir(exist_ok=True,parents=True)').replace("Native/build/libsm_native.so","Native/build/libsm_boss_tracker.so")
exec(compile(s,'boss-marker-fixture','exec'))
l.sm_tracker_publish_bosses.argtypes=[C.POINTER(Snapshot),C.POINTER(C.c_uint8),C.c_uint32]
boot('encounters',False);assert l.sm_test_all_equipment()
assert l.sm_test_room(0x9dc7,128,640)
for _ in range(800):
 step()
 if l.sm_state()==8 and word(0x79b)==0x9dc7:break
else:raise AssertionError(('Spore Spawn room failed',l.sm_state(),hex(word(0x79b)),word(0xaf6),word(0xafa)))
wait(60)
l.sm_tracker_configure(1,1,1)
# Real oracle output using the same acquired inventory and boss bits as renderer.
def publish():
 snapshot=snap();inv=dict(items=snapshot.acquired_items,beams=snapshot.acquired_beams,health=snapshot.max_health,reserve=snapshot.max_reserve,missiles=snapshot.max_missiles,supers=snapshot.max_supers,powerBombs=snapshot.max_power_bombs,bosses=list(snapshot.boss_bits),doors=list(snapshot.opened_doors),collected=list(snapshot.collected))
 buf=C.create_string_buffer(262144)
 assert l.sm_tracker_evaluate(str(root/'Randomizer').encode(),json.dumps(dict(randomized=False,inventory=inv)).encode(),buf,len(buf))>0
 r=json.loads(buf.value);assert r['ok'],r
 item_states=(C.c_uint8*100)(*[c['state'] for c in r['checks']]);boss_states=(C.c_uint8*10)(*[c['state'] for c in r['bosses']])
 assert l.sm_tracker_publish(C.byref(snapshot),item_states,100)
 assert l.sm_tracker_publish_bosses(C.byref(snapshot),boss_states,10)
 return snapshot,boss_states,r
snapshot,states,result=publish();assert states[5]==1,result['bosses'][5]
step();capture('spore-minimap')
# Put Samus near the top of this 3-screen room so the boss center is on minimap.
put(0xafa,256+128);step();snapshot,states,result=publish();step()
for width,pixels in [(256,l.sm_ui_overlay),(400,l.sm_wide_overlay)]:
 ss=snap();mx=22-ss.map_x+(4 if width==400 else 2);my=3-ss.map_y+1
 assert 0<=mx<(7 if width==400 else 5) and 0<=my<(4 if width==400 else 3),(ss.map_x,ss.map_y)
 x=(336 if width==400 else 208)+mx*8+4;y=my*8+4
 data=C.string_at(pixels(),width*240*4)
 # The central white player cursor is intentionally preserved.
 if (ss.map_x,ss.map_y)==(22,3):x+=1
 pixel=data[(y*width+x)*4:(y*width+x)*4+3]
 assert pixel==bytes([32,255,32]),(width,pixel.hex())
capture('spore-minimap-near')
# Render fixture: enter the native pause transition directly. This test does
# not validate Start input during the boss entrance animation.
put(0x998,12)
for _ in range(400):
 step()
 if l.sm_state()==15:break
assert l.sm_state()==15,(l.sm_state(),word(0x9c2),hex(word(0x79b)))
snapshot,states,result=publish()
put(0xb1,22*8+4-128);put(0xb3,(3*8+4-112)&65535);step()
snapshot,states,result=publish();step()
capture('spore-pause-green')
for width,pixels in [(256,l.sm_ui_overlay),(400,l.sm_wide_overlay)]:
 sx=C.c_int16(word(0xb1)).value;sy=C.c_int16(word(0xb3)).value
 x=22*8+4-sx+(width-256)//2;y=3*8+4-sy
 data=C.string_at(pixels(),width*240*4)
 assert data[(y*width+x)*4:(y*width+x)*4+3]==bytes([32,255,32]),(width,x,y)
# A victory invalidates old results before the refreshed gray marker appears.
old_snapshot=snapshot
C.memmove(ram+0xd829,bytes([read(0xd829,1)[0]|2]),1)
assert not l.sm_tracker_publish_bosses(C.byref(old_snapshot),states,10)
snapshot,states,result=publish();assert states[5]==0
assert not l.sm_tracker_publish_bosses(C.byref(snapshot),states,9)
bad=(C.c_uint8*10)(*states);bad[5]=3
assert not l.sm_tracker_publish_bosses(C.byref(snapshot),bad,10)
step();capture('spore-pause-defeated')
# Encounter markers do not count as extra item pickups.
for a in range(6):assert l.sm_tracker_area_count(a,3)==sum(c['area']==a for c in geometry)
l.sm_shutdown()
print('BOSS_MARKERS_NATIVE_PAUSE_MINIMAP_4X3_WIDE_STALE_DEFEAT_COUNTS_PASS')
