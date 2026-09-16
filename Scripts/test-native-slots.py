#!/usr/bin/env python3
"""Isolated, headless A/B/C integration validation on gaming-pc."""
import ctypes as C,hashlib,json,sys,os
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert (root/'ISOLATED_TEST_DIRECTORY').is_file();os.chdir(root)
lib=C.CDLL(str(root/'Native/build/libsm_native.so'))
class Item(C.Structure):_fields_=[('address',C.c_uint32),('plm',C.c_uint16)]
Callback=C.CFUNCTYPE(C.c_int,C.c_int,C.c_int,C.c_int,C.c_void_p)
lib.sm_slots_enable.argtypes=[Callback,C.c_void_p]
lib.sm_slots_set.argtypes=[C.c_int,C.c_int,C.c_int,C.POINTER(Item),C.c_int,C.c_char_p]
lib.sm_slots_copy_sram.argtypes=[C.POINTER(C.c_uint8),C.c_int]
lib.sm_generation_commit.argtypes=[C.POINTER(Item),C.c_int,C.c_char_p,C.c_char_p]
lib.sm_init.argtypes=[C.c_char_p,C.c_char_p];lib.sm_error.restype=C.c_char_p
lib.sm_pixels.restype=C.POINTER(C.c_ubyte);lib.sm_simulation_ram.restype=C.POINTER(C.c_ubyte)
lib.sm_seed_fingerprint.restype=C.c_char_p
lib.sm_stats_value.argtypes=[C.c_int];lib.sm_stats_value.restype=C.c_uint64
class StatsRecord(C.Structure):
 _fields_=[('seed',C.c_char*65),('started',C.c_uint8),('partial',C.c_uint8),('finished',C.c_uint8),('value',C.c_uint64*48)]
class StatsDisk(C.Structure):_fields_=[('magic',C.c_char*8),('slots',StatsRecord*3),('checksum',C.c_uint64)]
def stats_disk():return StatsDisk.from_buffer_copy(Path(str(sram)+'.stats').read_bytes())
out=root/'slot-results';out.mkdir(exist_ok=True);sram=out/'bank.sram.dat';sram.unlink(missing_ok=True);Path(str(sram)+'.stats').unlink(missing_ok=True)
rom=next((root/'roms').glob('*.sfc'));original=rom.read_bytes();romhash=hashlib.sha256(original).hexdigest()
plans=[json.loads(p.read_text())['manifest'] for p in sorted((root/'results').glob('seed-*.json'))]
print(plans[0].keys(),flush=True)
slots=[None,None,None];modes=[0,0,0];events=[];fail_save=False
def data(n):
 p=plans[n];return (Item*100)(*[Item(i['address'],i['plm']) for i in p['placements']]),p['sha256'].encode()
def register(i):
 if slots[i] is None:assert lib.sm_slots_set(i,modes[i],not modes[i],None,0,b'')
 else:
  items,h=data(slots[i]);assert lib.sm_slots_set(i,1,1,items,100,h)
@Callback
def action(a,s,o,ctx):
 events.append((a,s,o))
 if a==0:modes[s]=o;register(s)
 elif a==2:modes[o]=modes[s];slots[o]=slots[s];register(o)
 elif a==3:modes[s]=0;slots[s]=None;register(s)
 elif a==4:
  if fail_save:return 0
  buf=(C.c_uint8*8192)();assert lib.sm_slots_copy_sram(buf,8192)==8192;sram.write_bytes(bytes(buf))
 return 1
def word(o):return int.from_bytes(C.string_at(C.addressof(lib.sm_simulation_ram().contents)+o,2),'little')
def step(b=0):assert lib.sm_step(b),lib.sm_error()
def wait(n=3):
 for _ in range(n):step()
def press(b):step(b);step()
def capture(name):
 from PIL import Image
 Image.frombytes('RGBA',(256,240),C.string_at(lib.sm_pixels(),256*240*4),'raw','BGRA').save(out/(name+'.png'))
def until(pred,n=9000,boot=False):
 for i in range(n):
  step(8 if boot and i>180 and i%120<2 else 0)
  if pred():return
 raise AssertionError((lib.sm_state(),word(0x727),word(0xde2)))
def options():return lib.sm_state()==2 and word(0xde2)==3
def boot():
 assert lib.sm_seed_clear();assert lib.sm_init(str(rom).encode(),str(sram).encode()),lib.sm_error()
 lib.sm_slots_enable(action,None)
 for i in range(3):register(i)
 until(lambda:lib.sm_state()==4 and word(0x727)==4,boot=True);wait()
def choose(s):
 while word(0x952)!=s:press(32)
 press(8);until(options);wait()
 assert lib.sm_slots_current()==s
 assert lib.sm_seed_fingerprint()==(data(slots[s])[1] if slots[s] is not None else b'')
def row(n):
 for _ in range(8):
  if word(0x99e)==n:break
  press(32)
 assert word(0x99e)==n,(n,word(0x99e))
 wait()
def gray(row):
 cells=[word(0x3000+2*(row*32+x)) for x in range(1,31)]
 return all(v&0x400 for v in cells if (v&1023)!=0x0f)
def snapshot_slot(i):
 offset=int.from_bytes(original[0x812b%0x8000+0x8000+i*2:0x812b%0x8000+0x8000+i*2+2],'little')
 return sram.read_bytes()[offset:offset+1628]
boot();choose(0);assert not gray(23) and gray(18) and gray(15);capture('vanilla-options')
row(1);press(32);assert word(0x99e)==0;press(32);assert word(0x99e)==1
assert not lib.sm_generation_take_request() and not events
# Toggle both ways; no language flags changed, no generation until confirmation.
row(1);press(128);press(128);press(128);wait();assert lib.sm_generation_state()==0 and gray(23) and not gray(15)
press(128);wait();assert lib.sm_generation_state()==-1 and not gray(23)
lib.sm_shutdown()
for s,plan in [(1,0),(2,1)]:
 boot();choose(s);row(1);press(128);press(128);press(128);wait();assert lib.sm_generation_state()==0
 row(2);press(8);assert events[-1]==(1,s,0)
 row(0);press(8);assert options()
 row(4);press(8);assert lib.sm_generation_take_request()==1
 items,h=data(plan);before=sram.read_bytes()
 assert not lib.sm_generation_commit(items,100,h,b'/tmp/wrong-bank.sram'), 'Accepted another bank'
 if s==1:
  fail_save=True
  assert not lib.sm_generation_commit(items,100,h,str(sram).encode())
  assert sram.read_bytes()==before and not lib.sm_seed_active()
  lib.sm_generation_fail(1);wait();assert lib.sm_generation_state()==3
  fail_save=False;press(8);assert lib.sm_generation_take_request()==1
 assert lib.sm_generation_commit(items,100,h,str(sram).encode()),lib.sm_error()
 slots[s]=plan
 wait();assert lib.sm_generation_state()==2 and not gray(23) and gray(18)
 capture(f'generated-{s}')
 assert sram.stat().st_size==8192 and snapshot_slot(s)!=bytes(1628),'No real initial SRAM data'
 saved=snapshot_slot(s)
 row(1);count=len(events);press(8);assert len(events)==count,'Changed generated mode'
 row(4);press(8);assert not lib.sm_generation_take_request()
 lib.sm_shutdown();boot();capture(f'file-select-{s}');choose(s)
 assert snapshot_slot(s)==saved,'Reload mutated progress'
 assert lib.sm_generation_state()==2 and gray(18)
 row(0);press(8)
 # Existing save enters the original area map; confirm until normal gameplay.
 until(lambda:lib.sm_state()==8,boot=True);wait(5)
 assert lib.sm_seed_fingerprint()==h
 assert word(0x79b)==0x91f8,(hex(word(0x79b)),lib.sm_state())
 assert word(0x9c4)==99
 assert not lib.sm_stats_partial(), "Fresh generated save was marked as old history"
 assert lib.sm_stats_value(41)==0, "First Start Game counted as a reset"
 lib.sm_shutdown()
boot();capture('mixed-file-select')
# Copy B to A through the original confirmation menu, then clear A only.
while word(0x952)!=3:press(32)
press(8);until(lambda:word(0x727)==8);assert word(0x1997+30)==1
press(8);until(lambda:word(0x727)==10);assert word(0x1997+30)==0
press(8);until(lambda:word(0x727)==12);press(8);until(lambda:word(0x727)==14)
assert (2,1,0) in events and slots[0]==slots[1]
assert snapshot_slot(0)==snapshot_slot(1)
assert bytes(stats_disk().slots[0])==bytes(stats_disk().slots[1]), "Copied slot lost statistics"
press(8);until(lambda:word(0x727)==4);wait();capture('copied-file-select')
while word(0x952)!=4:press(32)
press(8);until(lambda:word(0x727)==22);assert word(0x1997+30)==0
press(8);until(lambda:word(0x727)==24);press(8);until(lambda:word(0x727)==26)
assert (3,0,0) in events and slots[0] is None and slots[1]==0 and slots[2]==1
disk=stats_disk();assert bytes(disk.slots[0])==bytes(C.sizeof(StatsRecord))
assert disk.slots[1].started==disk.slots[2].started==1
press(8);until(lambda:word(0x727)==4);wait();capture('cleared-file-select')
choose(0);assert not lib.sm_seed_active() and not gray(23) and gray(18)
for i,p in enumerate(sorted(plans[0]['placements'],key=lambda p:p['address'])):
 assert lib.sm_seed_rom_item(i)==int.from_bytes(original[p['address']:p['address']+2],'little')
assert lib.sm_generation_state()==-1
lib.sm_shutdown()
assert hashlib.sha256(rom.read_bytes()).hexdigest()==romhash
(out/'verification.json').write_text(json.dumps(dict(passed=True,host=os.uname().nodename,slots=['vanilla',plans[0]['seed'],plans[1]['seed']],events=events,nativeSha256=hashlib.sha256((root/'Native/build/libsm_native.so').read_bytes()).hexdigest(),freshRunStats=True,statisticsCopyAndClear=True,scope='Native menu, seed switching, autosave/reload and statistics; not Unreal profile persistence'),indent=2))
print('ALL SLOT CHECKS PASSED',flush=True)
