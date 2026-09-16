#!/usr/bin/env python3
"""Source BCD clocks, objective-triggered escape and native common escape routines."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-objectives.py').read_text().split("for goal in audit['goals']:")[0]
source=source.replace('objectives/libsm_native.so','escape/libsm_native.so').replace("root/'objectives/native'","root/'escape/native'")
exec(compile(source,'escape-clock-fixture','exec'))
class Clock(C.Structure):
 _fields_=[('version',C.c_uint32),('size',C.c_uint32),('flags',C.c_uint16),('reserved',C.c_uint16),('timer',C.c_uint16),('half_timer',C.c_uint16),('timers',C.c_uint16*10),('half_timers',C.c_uint16*10)]
assert C.sizeof(Clock)==56
configure=l.sm_escape_clock_configure;configure.argtypes=[C.c_int,C.POINTER(Clock)]
active=routine('sm_escape_active',C.c_int);enabled=routine('sm_escape_enabled',C.c_int)
timer=routine('ProcessTimer_MotherBrainStart',C.c_uint8)
extended=routine('sm_escape_extended',C.c_int)
room_setup=routine('RunRoomSetupCode',None)
load_header=routine('LoadStateHeader',None)
save_station=routine('PlmInstr_ActivateSaveStationAndGotoIfNo',C.c_void_p,C.c_void_p,C.c_uint16)
original_configure=globals()['config']
# The shared fixture's commit() calls its area configure helper by this name.
clock_configure=configure
configure=lambda slot,mapping:l.sm_areas_configure(slot,(C.c_uint8*len(mapping))(*mapping),len(mapping),catalog['sha256'].encode())
results=[]
for case in range(3):
 data=json.loads((root/f'escape/capture/seed-{case:02d}.json').read_text())
 spans={int(a):b for patch in data['patches'] if patch['name']=='Escape_Timer' for a,b in patch['data'].items()}
 p=Clock();p.version=1;p.size=C.sizeof(p);p.flags=data['clock']['flags']
 def bcd(a):return spans[a][0]|spans[a][1]<<8
 p.timer=bcd(7713)
 if case==0:p.half_timer=bcd(1110264)
 else:
  p.timers[:]=[spans[1110224][i*2]|spans[1110224][i*2+1]<<8 for i in range(10)]
  p.half_timers[:]=[spans[1110244][i*2]|spans[1110244][i*2+1]<<8 for i in range(10)]
 assert clock_configure(case,C.byref(p));commit(plan([byname['nothing']],flags=1),case);clear();assert enabled()
 if case:
  # Nothing must wait until Crateria, not auto-trigger at a remote start.
  put(0x79b,0xa7de);put(0x79f,2);frame();assert not event(10) and not active()
  put(0x79b,0x91f8);put(0x79f,0)
  inventory=read(0x9a2,10);frame()
  assert event(10) and event(11) and event(14) and event(160) and active()
  assert read(0xd8b0,32)==b'\xff'*32 and read(0xd828,8)==b'\xff'*8
  assert read(0x9a2,10)==inventory and word(0x943)==2
  assert word(0x183e)==0x15 and word(0x1840)==0xffff
  before=read(0,131072);frame();assert word(0x943)==2 # trigger is one-shot
  pointer=C.create_string_buffer(bytes.fromhex('34 92'));before=read(0,131072)
  assert save_station(C.addressof(pointer),0)&65535==0x9234
  assert read(0,131072)==before
 else:
  frame();assert event(10) and not active()
  mark(14);put(0x78d,0xaaec);routine('RunDoorSetupCode',None)()
  assert read(0xd8b0,32)==b'\xff'*32 and event(160)
 # Test actual translated timer start against every source-written region.
 for r in range(1,11):
  row=next(x for x in map_audit['rooms'] if x['owners'][0]==r)
  put(0x79b,row['room']);timer()
  assert word(0x946)==(p.timers[r-1] if case else p.timer)
  assert word(0x1ff4e)==(p.half_timers[r-1] if case else p.half_timer)
  assert word(0x943)==0x8003
 # Only original escape rooms skip the extended shake/explosion helper.
 for area,room,wanted in [(5,0xdd58,0),(0,0x91f8,0),(0,0x92fd,0),(0,0x9879,0),(0,0x9804,0),(0,0x96ba,0),(0,0x93fe,1),(2,0xa7de,1)]:
  put(0x79f,area);put(0x79b,room);assert extended()==wanted
 # Source enemy-removal table keeps elevators and special machinery alive.
 enemy_audit=json.loads((root/'escape/data-audit.json').read_text())
 header=routine('sm_escape_load_header',None)
 for row in enemy_audit['enemyTable']+[dict(room=0xa999,population=0x85a9,graphics=0x80eb)]:
  put(0x79f,2);put(0x79b,row['room']);put(0x7cf,0x1234);put(0x7d1,0x5678);put(0x7c9,7);put(0x7cb,9)
  header();assert word(0x7c9)==word(0x7cb)==0
  assert (word(0x7cf),word(0x7d1))==((row['population'],row['graphics']) if case==0 else (0x1234,0x5678))
 for pop in enemy_audit['populations']:assert C.string_at(romptr+pop['address'],19)==bytes(pop['data'])
 # Hyper opens ammo blocks after MB; actual Plasma cannot do so without Tourian.
 put(0xc18,8);put(0xdde,0)
 hyper=routine('sm_escape_hyper',C.c_int,C.c_uint)
 assert bool(hyper(0))==(case==0)
 pb=routine('PlmSetup_RespawningPowerBombBlock',C.c_uint8,C.c_uint16)
 put(0x1c37,0x1234);put(0x1c87,0);put(0x10002,0xc057);pb(0)
 assert (word(0x1c37)!=0)==(case==0)
 results.append(dict(case=case,timer=p.timer,timers=list(p.timers),flags=p.flags))
# Invalid clock values cannot replace a previously configured plan.
bad=Clock.from_buffer_copy(p);bad.timers[0]=0x1a00;assert not clock_configure(2,C.byref(bad))
bad=Clock.from_buffer_copy(p);bad.flags=3;assert not clock_configure(2,C.byref(bad))
for slot in range(3):assert clock_configure(slot,None)
commit(plan([byname['nothing']]));clear();assert not enabled()
timer();assert word(0x946)==0x300
pristine=rom.read_bytes()
for pop in enemy_audit['populations']:assert C.string_at(romptr+pop['address'],19)==pristine[pop['address']:pop['address']+19]
assert l.sm_cpu_opcodes()==0
(root/'escape/clock-native-results.json').write_text(json.dumps(dict(passed=True,sourceClocks=results,nativeTimerCases=30,enemyHeaderCases=48,elevatorPopulationBytes=114,populationsRestored=True,saveBlockedDuringEscape=True,objectiveTriggered=True,hyperRestricted=True,scope='Common behavior only; routing, room data, solver and production activation remain pending.',nativeSha256=hashlib.sha256((root/'escape/libsm_native.so').read_bytes()).hexdigest()),indent=2)+'\n')
l.sm_shutdown();print('ESCAPE_CLOCK_NATIVE_PASS',len(results),flush=True)
