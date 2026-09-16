#!/usr/bin/env python3
"""Exercise actual native Area Select from pause, with seed-aware counters."""
import sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
src=(root/'Scripts/test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
src=src.replace("l=C.CDLL(str(root/'Native/build/libsm_native.so'))","l=C.CDLL(str(root/'map-browser/libsm_native.so'))")
exec(compile(src,'tracker-fixture','exec'))
out=root/'map-browser/SMTests';out.mkdir(exist_ok=True)
l.sm_slots_copy_sram.argtypes=[C.c_void_p,C.c_int]
def hold(b,n=8):
 for _ in range(n):step(b)
 step()
def world():
 s=snap();return (s.slot,s.room,s.area,s.map_x,s.map_y,s.acquired_items,s.active_items,s.acquired_beams,s.active_beams,bytes(s.item_bits),bytes(s.boss_bits),bytes(s.events),bytes(s.opened_doors),bytes(s.explored_current),bytes(s.explored_saved),bytes(s.map_stations))
def sram():
 b=C.create_string_buffer(8192);assert l.sm_slots_copy_sram(b,8192)==8192;return b.raw
boot('randomized',True);l.sm_tracker_configure(1,1,1);query(snap())
assert l.sm_test_seed_room(1);wait(550);hold(8);wait(100)
assert l.sm_state()==15 and not l.sm_pause_data(12)
states=query(snap());baseline=world();bank=sram();scroll=(word(0xb1),word(0xb3));capture('original-pause')
assert l.sm_map_browser_state(2)==63
hold(4);wait(8);assert l.sm_map_browser_state(0)==1
assert world()==baseline and sram()==bank
capture('overview')
seen=[]
for _ in range(6):
 area=l.sm_map_browser_state(1)
 counts=[l.sm_tracker_area_count(area,f) for f in [1,2,3]]
 expected_total=sum(c['area']==area for c in geometry)
 assert counts[2]==expected_total and 0<=counts[0]<=counts[1]<=counts[2],(area,counts)
 assert counts[0]==sum(c['area']==area and states[i]==1 and not snap().collected[i] for i,c in enumerate(geometry))
 hold(256);wait(8);assert l.sm_map_browser_state(0)==2
 assert l.sm_map_browser_state(3)==area and l.sm_area()==baseline[2]
 assert world()==baseline and sram()==bank
 capture(f'area-{area}')
 # The embedded marker renderer must use the viewed area, never Samus's area.
 ui=C.string_at(l.sm_wide_overlay(),400*240*4);sx=C.c_int16(word(0xb1)).value;sy=C.c_int16(word(0xb3)).value
 groups={}
 for i,c in enumerate(geometry):
  if c['area']==area:
   key=c['mapX'],c['mapY'];groups[key]=groups.get(key,0)|states[i]
 markers=0
 for (mx,my),state in groups.items():
  x=mx*8+4-sx+72;y=my*8+4-sy
  if state not in [1,2,4,8] or x-3<8 or x+3>=392 or y-3<48 or y+3>=192:continue
  p=ui[(y*400+x)*4:(y*400+x)*4+4]
  assert (p[2]<<16|p[1]<<8|p[0])==l.sm_tracker_color(state),(area,mx,my,state)
  markers+=1
 if area!=5:assert markers>0,area
 hold(128,24);hold(32,24);assert world()==baseline
 hold(1);wait(8);assert l.sm_map_browser_state(0)==1
 assert l.sm_map_browser_state(1)==area
 seen.append(dict(area=area,counts=counts,verifiedMarkers=markers));hold(32);wait(3)
assert len({c['area'] for c in seen})==6
# B closes native overview to actual current map, restoring its scroll.
hold(1);wait(8);assert l.sm_map_browser_state(0)==0
assert (word(0xb1),word(0xb3))==scroll
assert world()==baseline and sram()==bank
capture('returned-current-map')
# Start selects a region from overview; a subsequent Start resumes gameplay.
hold(4);hold(32);hold(8);wait(8)
assert l.sm_state()==15 and l.sm_map_browser_state(0)==2
hold(8);wait(120);assert l.sm_state()==8 and l.sm_area()==baseline[2]
assert sram()==bank
# Enter equipment directly from a remote map, then return to current map.
hold(8);wait(100);hold(4);hold(32);hold(256);wait(8);hold(2048);wait(110)
assert l.sm_pause_data(12)==1 and l.sm_map_browser_state(0)==0
hold(1024);wait(110);assert l.sm_pause_data(12)==0
assert l.sm_map_browser_state(3)==baseline[2]
# Collection changes update remaining immediately and invalidate accessibility.
old_bits=read(0xd870,64);old_snapshot=snap()
idx=next(i for i,c in enumerate(geometry) if c['area']==1 and not old_snapshot.collected[i])
bit=geometry[idx]['collectionBit'];off=0xd870+(bit>>3)
C.c_uint8.from_address(ram+off).value|=1<<(bit&7)
assert l.sm_tracker_area_count(1,1)==-1 and l.sm_tracker_area_count(1,2)==30
assert not l.sm_tracker_publish(C.byref(old_snapshot),states,100)
C.memmove(ram+0xd870,old_bits,64);query(snap())
assert not l.sm_cpu_opcodes();l.sm_shutdown()
# Vanilla: visited areas only by default. Tracker opt-in reveals all six.
boot('vanilla',False);hold(8);wait(100)
assert l.sm_state()==15
assert l.sm_tracker_area_count(0,0)==0
mask=l.sm_map_browser_state(2);assert mask&1 and mask!=63
hold(4);wait(8);capture('vanilla-overview')
assert l.sm_map_browser_state(0)==1
l.sm_tracker_configure(1,1,1);assert l.sm_map_browser_state(2)==63
# Pending results must never pretend zero accessibility.
assert l.sm_tracker_area_count(0,1)==-1;capture('pending-overview')
query(snap());wait(4);capture('vanilla-tracker-overview')
# Opt-out while viewing an unvisited region must return to the real area.
hold(32);hold(256);wait(8)
assert l.sm_map_browser_state(0)==2 and l.sm_map_browser_state(3)!=l.sm_area()
l.sm_tracker_configure(0,1,1);wait(8)
assert l.sm_map_browser_state(0)==0 and l.sm_map_browser_state(3)==l.sm_area()
# Also cover disabling it directly on the overview.
l.sm_tracker_configure(1,1,1);hold(4);hold(32)
l.sm_tracker_configure(0,1,1);wait(8)
assert l.sm_map_browser_state(1)==l.sm_area()
hold(1);wait(8);capture('vanilla-restored-map')
l.sm_shutdown()
report=dict(passed=True,areas=seen,vanillaVisibleMask=mask,allSixRegions=True,sramUnchanged=True,worldUnchanged=True,nativeEquipmentReturn=True,nativeResume=True,pendingIsUnknown=True,cpuOpcodes=0)
(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n');print('SM_MAP_BROWSER_PASS',json.dumps(report),flush=True)
