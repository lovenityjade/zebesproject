#!/usr/bin/env python3
"""Isolated, headless A/B/C integration validation on gaming-pc."""
import ctypes as C,hashlib,json,sys,os
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file();os.chdir(root)
libpath=root/(sys.argv[4] if len(sys.argv)>4 else 'world-data/libsm_native.so')
lib=C.CDLL(str(libpath))
class Item(C.Structure):_fields_=[('address',C.c_uint32),('plm',C.c_uint16),('kind',C.c_uint16)]
class Indicator(C.Structure):_fields_=[('location',C.c_uint16),('plm',C.c_uint16)]
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
fixtures=sys.argv[3] if len(sys.argv)>3 else 'start-seeds'
assert Path(fixtures).name==fixtures
out=root/('start-slot-results'+('-'+sys.argv[2] if len(sys.argv)>2 else '')+('-'+fixtures if fixtures!='start-seeds' else ''));out.mkdir(exist_ok=True);sram=out/'bank.sram.dat';sram.unlink(missing_ok=True);Path(str(sram)+'.stats').unlink(missing_ok=True)
rom=next((root/'roms').glob('*.sfc'));original=rom.read_bytes();romhash=hashlib.sha256(original).hexdigest()
plans=[json.loads(p.read_text())['manifest'] for p in [root/fixtures/f'seed-{i}.json' for i in (sys.argv[2].split(',') if len(sys.argv)>2 else ['02','10'])]]
print(plans[0].keys(),flush=True)
slots=[None,None,None];modes=[0,0,0];events=[];fail_save=False;objective_progress=[]
def data(n):
 p=plans[n];return (Item*100)(*[Item(i['address'],i['plm'],i.get('kind',0)) for i in p['placements']]),p['sha256'].encode()
lib.sm_start_configure.argtypes=[C.c_int,C.c_int,C.POINTER(C.c_uint8),C.c_int,C.c_char_p]
lib.sm_world_data_catalog_sha256.restype=C.c_char_p
lib.sm_connections_configure.argtypes=[C.c_int,C.POINTER(C.c_uint8),C.c_int,C.c_char_p]
lib.sm_connections_catalog_sha256.restype=C.c_char_p
lib.sm_doors_configure.argtypes=[C.c_int,C.POINTER(C.c_uint8),C.c_int,C.c_char_p]
lib.sm_doors_catalog_sha256.restype=C.c_char_p
class ObjectivePlan(C.Structure):
 _fields_=[('version',C.c_uint32),('size',C.c_uint32)]+[(n,C.c_uint16) for n in ['count','required','flags','item_mask','beam_mask']]+[(n,C.c_uint8*s) for n,s in [('goals',18),('item_counted',100),('area_counted',100)]]+[('enemy_totals',C.c_uint16*6),('map_totals',C.c_uint16*12)]
def configure_objectives(i,plan):
 if not hasattr(lib,'sm_objectives_configure'):return
 lib.sm_objectives_configure.argtypes=[C.c_int,C.POINTER(ObjectivePlan),C.c_char_p];lib.sm_objectives_catalog_sha256.restype=C.c_char_p
 data=plans[plan]['nativeContext'].get('objectives') if plan is not None else None
 value=ObjectivePlan()
 if data:
  value.version=1;value.size=C.sizeof(value);value.count=len(data['goals']);value.required=data['required'];value.flags=data['flags']
  value.item_mask=data['itemMask'];value.beam_mask=data['beamMask'];value.goals[:value.count]=data['goals']
  value.item_counted[:]=data['itemCounted'];value.area_counted[:]=data['areaCounted']
  value.enemy_totals[:]=data['enemyTotals'];value.map_totals[:]=data['mapTotals']
 assert lib.sm_objectives_configure(i,C.byref(value) if data else None,data['catalogSha256'].encode() if data else lib.sm_objectives_catalog_sha256())
def start_config(i,plan=None):
 topology=plans[plan]['nativeContext'].get('topology',{}) if plan is not None else {}
 if hasattr(lib,'sm_areas_configure'):
  lib.sm_areas_configure.argtypes=[C.c_int,C.POINTER(C.c_uint8),C.c_int,C.c_char_p];lib.sm_areas_catalog_sha256.restype=C.c_char_p
  native=topology.get('nativeAreas',dict(destinations=[],catalogSha256=lib.sm_areas_catalog_sha256().decode()))
  table=(C.c_uint8*len(native['destinations']))(*native['destinations'])
  assert lib.sm_areas_configure(i,table,len(table),native['catalogSha256'].encode())
 configure_objectives(i,plan)
 colors=plans[plan]['nativeContext'].get('doorColors') if plan is not None else None
 colors=colors or dict(colors=[],catalogSha256=lib.sm_doors_catalog_sha256().decode())
 table=(C.c_uint8*len(colors['colors']))(*colors['colors'])
 assert lib.sm_doors_configure(i,table,len(table),colors['catalogSha256'].encode())
 topology=plans[plan]['nativeContext'].get('topology',{}) if plan is not None else {}
 native=topology.get('native',dict(destinations=[],catalogSha256=lib.sm_connections_catalog_sha256().decode()))
 targets=(C.c_uint8*len(native['destinations']))(*native['destinations'])
 assert lib.sm_connections_configure(i,targets,len(targets),native['catalogSha256'].encode())
 w=plans[plan]['nativeContext']['world'] if plan is not None else dict(startSpawn=0,dataPatchIds=[],catalogSha256=lib.sm_world_data_catalog_sha256().decode())
 patches=(C.c_uint8*len(w['dataPatchIds']))(*w['dataPatchIds'])
 entries=(Indicator*len(w.get('indicators',[])))(*[Indicator(e['locationId'],e['plm']) for e in w.get('indicators',[])])
 lib.sm_start_configure_world.argtypes=lib.sm_start_configure.argtypes+[C.POINTER(Indicator),C.c_int]
 assert lib.sm_start_configure_world(i,w['startSpawn'],patches,len(patches),w['catalogSha256'].encode(),entries,len(entries))
 if hasattr(lib,'sm_start_configure_initial_doors'):
  lib.sm_start_configure_initial_doors.argtypes=[C.c_int,C.c_int,C.POINTER(C.c_uint8),C.c_int]
  opened=w.get('initialDoors',{}).get('opened',[]);doors=(C.c_uint8*len(opened))(*opened)
  assert lib.sm_start_configure_initial_doors(i,w['startSpawn'],doors,len(doors))
def register(i):
 start_config(i,slots[i])
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
def verify_routing(plan):
 if hasattr(lib,'sm_objectives_state'):
  obj=plans[plan]['nativeContext'].get('objectives') if plan is not None else None
  assert lib.sm_objectives_state(0)==(len(obj['goals']) if obj else 0),('slot objectives',plan)
  if obj:assert lib.sm_objectives_state(1)==obj['required'] and lib.sm_objectives_state(6)==obj['flags']
 import subprocess
 path=libpath
 symbols={p[-1]:int(p[0],16) for line in subprocess.check_output(['nm','-an',str(path)],text=True).splitlines() if len(p:=line.split())==3 and p[1].lower() in ('t','b','d')}
 base=C.cast(lib.sm_init,C.c_void_p).value-symbols['sm_init'];romptr=C.c_void_p.from_address(base+symbols['g_rom']).value
 catalog=json.loads((root/'Randomizer/native_connections.json').read_text())
 topology=plans[plan]['nativeContext'].get('topology',{}) if plan is not None else {}
 mapping=topology.get('native',{}).get('destinations',[])
 for i,ap in enumerate(catalog['accessPoints']):
  address=0x10000+ap['exit']['DoorPtr'];expected=original[address:address+12]
  if mapping:
   c=catalog['connections'][i][mapping[i]];room=c['room']
   expected=room.to_bytes(2,'little')+bytes([c['bitFlag'],c['direction'],*c['cap'],*c['screen']])+c['distance'].to_bytes(2,'little')+c['asm'].to_bytes(2,'little')
  assert C.string_at(romptr+address,12)==expected,('slot routing',plan,i)
 area_catalog=json.loads((root/'Randomizer/native_areas.json').read_text())
 mapping=topology.get('nativeAreas',{}).get('destinations',[])
 for i,ap in enumerate(area_catalog['accessPoints']):
  address=0x10000+ap['exit']['DoorPtr'];expected=original[address:address+12]
  if mapping:
   c=area_catalog['connections'][i][mapping[i]];room=c['room']
   expected=room.to_bytes(2,'little')+bytes([c['bitFlag'],c['direction'],*c['cap'],*c['screen']])+c['distance'].to_bytes(2,'little')+c['asm'].to_bytes(2,'little')
  assert C.string_at(romptr+address,12)==expected,('slot area routing',plan,i)
 if plan is not None and 'doorColors' in plans[plan]['nativeContext']:
  catalog=json.loads((root/'Randomizer/native_door_colors.json').read_text())
  sys.path.insert(0,str(root/'Randomizer/upstream'))
  from utils.doorsmanager import colors2plm
  for loc,value in zip(catalog['locations'],plans[plan]['nativeContext']['doorColors']['colors']):
   if value:
    expected=colors2plm[catalog['colors'][value]][loc['facing']].to_bytes(2,'little')
    assert C.string_at(romptr+loc['address'],2)==expected,('door color slot',plan,loc['name'])
    if value==4:assert C.string_at(romptr+loc['address']+5,1)==b'\x90'
def choose(s):
 while word(0x952)!=s:press(32)
 press(8);until(options);wait()
 assert lib.sm_slots_current()==s
 verify_routing(slots[s])
 assert lib.sm_seed_fingerprint()==(data(slots[s])[1] if slots[s] is not None else b'')
def row(n):
 while word(0x99e)!=n:press(32)
 wait()
def gray(row):return bool(word(0x3000+2*(row*32+4))&0x400)
def snapshot_slot(i):
 offset=int.from_bytes(original[0x812b%0x8000+0x8000+i*2:0x812b%0x8000+0x8000+i*2+2],'little')
 return sram.read_bytes()[offset:offset+1628]
boot();choose(0);assert not gray(6) and gray(21) and gray(13);capture('vanilla-options')
row(4);press(8);assert not lib.sm_generation_take_request() and options()
row(2);press(8);assert not events
# Toggle both ways; no language flags changed, no generation until confirmation.
row(1);press(8);wait();assert lib.sm_generation_state()==0 and gray(6) and not gray(13)
press(8);wait();assert lib.sm_generation_state()==-1 and not gray(6)
lib.sm_shutdown()
for s,plan in [(1,0),(2,1)]:
 boot();choose(s);row(1);press(8);wait();assert lib.sm_generation_state()==0
 row(2);press(8);assert events[-1]==(1,s,0)
 row(0);press(8);assert options()
 row(4);press(8);assert lib.sm_generation_take_request()==1
 items,h=data(plan);before=sram.read_bytes();start_config(s,plan)
 ram_before=C.string_at(lib.sm_simulation_ram(),0x20000)
 assert not lib.sm_generation_commit(items,100,h,b'/tmp/wrong-bank.sram'), 'Accepted another bank'
 if s==1:
  fail_save=True
  assert not lib.sm_generation_commit(items,100,h,str(sram).encode())
  assert sram.read_bytes()==before and not lib.sm_seed_active()
  assert C.string_at(lib.sm_simulation_ram(),0x20000)==ram_before, "Failed commit changed RAM"
  lib.sm_generation_fail(1);wait();assert lib.sm_generation_state()==3
  fail_save=False;press(8);assert lib.sm_generation_take_request()==1
 assert lib.sm_generation_commit(items,100,h,str(sram).encode()),lib.sm_error()
 slots[s]=plan
 verify_routing(plan)
 wait();assert lib.sm_generation_state()==2 and not gray(6) and gray(21)
 capture(f'generated-{s}')
 assert sram.stat().st_size==8192 and snapshot_slot(s)!=bytes(1628),'No real initial SRAM data'
 saved=snapshot_slot(s)
 row(1);count=len(events);press(8);assert len(events)==count,'Changed generated mode'
 row(4);press(8);assert not lib.sm_generation_take_request()
 lib.sm_shutdown();boot();capture(f'file-select-{s}');choose(s)
 assert snapshot_slot(s)==saved,'Reload mutated progress'
 assert lib.sm_generation_state()==2 and gray(21)
 row(0);press(8)
 # Existing save enters the original area map; confirm until normal gameplay.
 until(lambda:lib.sm_state()==8,boot=True);wait(5)
 assert lib.sm_seed_fingerprint()==h
 assert word(0x79b)=={'Landing Site':0x91f8,'Ceres':0xdf45,'Gauntlet Top':0x99bd,'Mama Turtle':0xd055,'Aqueduct':0xd5a7,'Green Brinstar Elevator':0x9ad9,'Business Center':0xa7de,'Golden Four':0xa5ed,'Red Brinstar Elevator':0xa322}[plans[plan]['nativeContext']['start']],(hex(word(0x79b)),lib.sm_state())
 for bit in plans[plan]['nativeContext']['world'].get('initialDoors',{}).get('opened',[]):
  assert C.string_at(C.addressof(lib.sm_simulation_ram().contents)+0xd8b0+bit//8,1)[0]&(1<<(bit%8)),('Initial door not saved/reloaded',s,bit)
 assert word(0x9c4)==99
 assert not lib.sm_stats_partial(), "Fresh generated save was marked as old history"
 assert lib.sm_stats_value(41)==0, "First Start Game counted as a reset"
 if plans[plan]['nativeContext'].get('objectives'):
  assert lib.sm_objectives_state(2)==0, 'A new seed inherited completed goals'
  ptr=C.addressof(lib.sm_simulation_ram().contents);event=72 if s==1 else 80;address=0xd820+(event>>3)
  C.memmove(ptr+address,bytes([C.string_at(ptr+address,1)[0]|(1<<(event&7))]),1)
  wait(5);assert lib.sm_objectives_state(2)==1 and not lib.sm_objectives_state(4)
  import subprocess
  symbols={p[-1]:int(p[0],16) for line in subprocess.check_output(['nm','-an',str(libpath)],text=True).splitlines() if len(p:=line.split())==3 and p[1].lower()=='t'}
  base=C.cast(lib.sm_init,C.c_void_p).value-symbols['sm_init']
  C.CFUNCTYPE(None,C.c_uint16)(base+symbols['SaveToSram'])(s)
  lib.sm_shutdown();boot();choose(s);row(0);press(8);until(lambda:lib.sm_state()==8,boot=True)
  assert lib.sm_objectives_state(2)==1 and not lib.sm_objectives_state(4)
  objective_progress.append(dict(slot=s,seed=plans[plan]['seed'],bossEvent=event,completedAfterReload=1))
 if s==2:
  # Model a later save at the ship via the actual native SRAM routine. This
  # checks reload routing; it does not claim a walk from this start to the ship.
  import subprocess
  symbols={p[-1]:int(p[0],16) for line in subprocess.check_output(['nm','-an',str(libpath)],text=True).splitlines() if len(p:=line.split())==3 and p[1].lower()=='t'}
  base=C.cast(lib.sm_init,C.c_void_p).value-symbols['sm_init']
  save_native=C.CFUNCTYPE(None,C.c_uint16)(base+symbols['SaveToSram'])
  ptr=C.addressof(lib.sm_simulation_ram().contents)
  for off,value in [(0x79f,0),(0x78b,0),(0xd914,5)]:C.memmove(ptr+off,int(value).to_bytes(2,'little'),2)
  save_native(s);lib.sm_shutdown();boot();choose(s);row(0);press(8);until(lambda:lib.sm_state()==8,boot=True)
  assert word(0x79b)==0x91f8 and lib.sm_start_spawn(s)==plans[plan]['nativeContext']['spawn'], 'Reload reset to original seed start'
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
choose(0);assert not lib.sm_seed_active() and not gray(6) and gray(21)
for i,p in enumerate(sorted(plans[0]['placements'],key=lambda p:p['address'])):
 assert lib.sm_seed_rom_item(i)==int.from_bytes(original[p['address']:p['address']+2],'little')
assert lib.sm_generation_state()==-1
lib.sm_shutdown()
assert hashlib.sha256(rom.read_bytes()).hexdigest()==romhash
(out/'verification.json').write_text(json.dumps(dict(passed=True,host=os.uname().nodename,slots=['vanilla',plans[0]['seed'],plans[1]['seed']],events=events,nativeSha256=hashlib.sha256((libpath).read_bytes()).hexdigest(),freshRunStats=True,statisticsCopyAndClear=True,objectiveProgress=objective_progress,scope='Native menu, seed switching, autosave/reload and statistics; not Unreal profile persistence'),indent=2))
print('ALL SLOT CHECKS PASSED',flush=True)
