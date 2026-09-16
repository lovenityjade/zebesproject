#!/usr/bin/env python3
"""Real native routines and frames, isolated gaming-pc saves only."""
import sys, uuid, subprocess, struct
from pathlib import Path
root=Path(sys.argv[1]).resolve()
source=(root/'test-chozo-escape-lifecycle.py').read_text().split('for minutes in (3,5,6,7,10):')[0]
exec(compile(source,'chozo-fixture','exec'))
library=root/'Native/build/libsm_native.so'
symbols={line.split()[2]:int(line.split()[0],16) for line in subprocess.check_output(['nm','-an',str(library)],text=True).splitlines() if len(line.split())==3}
base=C.cast(l.sm_init,C.c_void_p).value-symbols['sm_init']
def routine(name,result,*args):return C.CFUNCTYPE(result,*args)(base+symbols[name])
damage=routine('Samus_DealDamage',None,C.c_uint16)
periodic=routine('Samus_HandlePeriodicDamage',None)
select_room=routine('HandleRoomDefStateSelect',None,C.c_uint16)
stop=routine('sm_relic_escape_stop',None)
label='stress-input-'+uuid.uuid4().hex
boot(label,True)
assert l.sm_test_all_equipment()
put(0x9a2,word(0x9a2)&~0x200);put(0x9a4,word(0x9a4)&~0x200)
put(0x9c2,999);damage(17);assert word(0x9c2)==982
original_checks=bytes(snap().collected)
off=0xd870+bit//8;C.memmove(ram+off,bytes([read(off,1)[0]|(1<<(bit&7))]),1)
step();wait(80)
assert l.sm_message_active() and not l.sm_relic_escape_active() and not word(0x943)
assert word(0x9a2)&0x200 and word(0x9a4)&0x200
assert sum(a!=b for a,b in zip(original_checks,bytes(snap().collected)))==1
capture('stress-warning')
for i in range(800):
 step(256 if i%60<5 else 0)
 if l.sm_relic_escape_active():break
else:raise AssertionError('Warning acknowledgement')
wait(220)
for n in (0,1,17,150,299,300,400,20000):
 put(0x9c2,999);damage(n)
 expected=999 if n==300 else max(0,999-n*2)
 assert word(0x9c2)==expected,(n,word(0x9c2),expected)
put(0x9c2,999);put(0xa78,1);damage(17);assert word(0x9c2)==999;put(0xa78,0)
gear=word(0x9a2)
for suit,loss in [(0,16),(1,8),(0x20,4)]:
 put(0x9a2,suit);put(0x9c2,999);put(0xa4e,0);put(0xa50,8);periodic()
 assert word(0x9c2)==999-loss,(suit,word(0x9c2))
put(0x9a2,gear);put(0x9c2,999)
# Original event remains set; only escape-specific room layouts are bypassed.
assert snap().events[1]&64
oldstate=word(0x7bb)
for room,escape in [(0x91f8,0x9261),(0x92fd,0x9348),(0x96ba,0x9705),(0x9804,0x984f),(0x9879,0x98c4)]:
 select_room(room);normal=word(0x7bb);assert normal!=escape,(hex(room),hex(normal))
 l.sm_relic_configure(1,0);select_room(room);assert word(0x7bb)==escape,(hex(room),hex(word(0x7bb)))
 l.sm_relic_configure(1,1)
put(0x7bb,oldstate)
# Native selection handlers still own availability, palettes and cancellation.
for _ in range(20):step(64) # Leave the ship's forward-facing spawn pose.
put(0x9d2,0)
for expected in [1,2,3,4,5,0]:
 press(0x2000);assert word(0x9d2)==expected,('RT',expected,word(0x9d2))
for expected in [5,4,3,2,1,0]:
 press(0x1000);assert word(0x9d2)==expected,('LT',expected,word(0x9d2))
step(0x2000)
for _ in range(20):step(0x2000)
assert word(0x9d2)==1;step()
press(0x3000);assert word(0x9d2)==1
put(0x9ca,0);put(0x9ce,0);press(0x2000);assert word(0x9d2)==4
press(0x1000);assert word(0x9d2)==1
press(4);assert word(0x9d2)==4,'Legacy Select changed'
press(2);assert word(0x9d2)==0,'Cancel changed'
# Test input filtering in a paused frame without advancing native pause state.
host=routine('sm_item_selection_input',C.c_uint16,C.c_uint16)
direction=routine('sm_item_selection_direction',C.c_int)
put(0x998,15);assert host(0x2004)==4 and direction()==0
put(0x998,8);host(0x2000);assert direction()==0
host(0);host(0x2000);assert direction()==1 and direction()==0;host(0)
# Exercise the real save-station prompt with the VARIA escape rules enabled.
class Clock(C.Structure):
 _fields_=[('version',C.c_uint32),('size',C.c_uint32),('flags',C.c_uint16),('reserved',C.c_uint16),('timer',C.c_uint16),('half_timer',C.c_uint16),('timers',C.c_uint16*10),('half_timers',C.c_uint16*10)]
clock=Clock();clock.version=1;clock.size=C.sizeof(clock);clock.timer=0x500;clock.half_timer=0x230
l.sm_escape_clock_configure.argtypes=[C.c_int,C.POINTER(Clock)]
assert l.sm_escape_clock_configure(1,C.byref(clock))
routine('sm_escape_activate',None,C.c_int)(1);routine('sm_escape_apply',None,C.c_int)(1)
assert routine('sm_escape_active',C.c_int)()
assert l.sm_teleport(1)
for _ in range(2000):
 step()
 if l.sm_state()==8 and l.sm_room()==0x93d5:break
wait(440)
for _ in range(25):step(64)
l.sm_set_refill_before_save(1)
put(0x9c2,37);put(0x9c6,2);put(0x9ca,1);put(0x9ce,1)
put(0xaf6,96);put(0xafa,120);put(0xb2e,0);put(0xb2c,0);put(0x1e75,0)
for _ in range(140):
 step()
 if l.sm_message_active():break
assert l.sm_message_active(),'Chozo escape disabled the save station'
wait(40);capture('escape-save-prompt')
events=[]
for i in range(700):
 step(256 if i%60<5 else 0)
 events += [l.sm_visual_state(17,j) for j in range(l.sm_visual_state(14,0))]
assert 142 in events,'Save never completed'
for current,maximum in [(0x9c2,0x9c4),(0x9c6,0x9c8),(0x9ca,0x9cc),(0x9ce,0x9d0)]:
 assert word(current)==word(maximum),(hex(current),word(current),word(maximum))
assert l.sm_relic_escape_active()
# Persist and migrate a legacy checkpoint, retaining time and warning once.
C.memmove(ram+0x945,bytes([0x42,0x17,0x02]),3)
assert l.sm_test_playtest(1,0)==1;l.sm_shutdown()
save=out/label/m['sha256']/'sram.dat.relic-escape'
b=bytearray(save.read_bytes());assert b[8+69+68]==2
b[8+69+68]=1
h=2166136261
for x in b[:-4]:h=((h^x)*16777619)&0xffffffff
struct.pack_into('<I',b,len(b)-4,h);save.write_bytes(b)
boot(label,True);assert not l.sm_relic_escape_active() and l.sm_message_active()
for i in range(800):
 step(256 if i%60<5 else 0)
 if l.sm_relic_escape_active():break
else:raise AssertionError('Legacy checkpoint warning')
assert read(0x947,1)[0]==2 and 0x14<=read(0x946,1)[0]<=0x17
assert word(0x9a2)&0x200 and word(0x9a4)&0x200
stop();put(0x9c2,999);damage(17);assert word(0x9c2)==982
assert not l.sm_cpu_opcodes();l.sm_shutdown()
print('CHOZO_WARNING_GIFT_DAMAGE_LAYOUT_INPUT_SAVE_REFILL_LEGACY_MIGRATION_PASS',flush=True)
