#!/usr/bin/env python3
"""Headless native-menu validation. Run only on the authorized test machine."""
import ctypes as C, hashlib, json, os, platform, sys
from pathlib import Path
ROOT=Path(sys.argv[1]).resolve()
assert (ROOT/'ISOLATED_TEST_DIRECTORY').exists(), 'Refuse to run against a user installation'
os.chdir(ROOT)
lib=C.CDLL(str(ROOT/'Native/build/libsm_native.so'))
class Item(C.Structure):_fields_=[('address',C.c_uint32),('plm',C.c_uint16)]
class Snapshot(C.Structure):
 _fields_=[('version',C.c_uint32),('size',C.c_uint32),('session',C.c_uint64),('revision',C.c_uint64)]+[(n,C.c_uint32) for n in ['frame','randomized','world_valid']]+[(n,C.c_uint16) for n in ['slot','state','room','area','map_x','map_y','acquired_items','active_items','acquired_beams','active_beams','max_health','max_missiles','max_supers','max_power_bombs','max_reserve']]+[(n,C.c_uint8*k) for n,k in [('collected',100),('item_bits',64),('boss_bits',8),('events',8),('opened_doors',64),('explored_current',256),('explored_saved',2048),('map_stations',8)]]+[('seed_fingerprint',C.c_char*65)]
lib.sm_init.argtypes=[C.c_char_p,C.c_char_p];lib.sm_error.restype=C.c_char_p
lib.sm_simulation_ram.restype=C.POINTER(C.c_uint8);lib.sm_pixels.restype=C.POINTER(C.c_uint8)
lib.sm_generation_commit.argtypes=[C.POINTER(Item),C.c_int,C.c_char_p,C.c_char_p]
lib.sm_randomizer_generate.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
rom=ROOT/'roms/Super Metroid (Japan, USA) (En,Ja).sfc';rom_hash=hashlib.sha256(rom.read_bytes()).hexdigest()
out=ROOT/'results';out.mkdir(exist_ok=True)
def word(offset):return int.from_bytes(C.string_at(C.addressof(lib.sm_simulation_ram().contents)+offset,2),'little')
def step(button=0):assert lib.sm_step(button),lib.sm_error()
def press(button):step(button);step()
def capture(name):
 from PIL import Image
 data=C.string_at(lib.sm_pixels(),256*240*4)
 Image.frombytes('RGBA',(256,240),data,'raw','BGRA').save(out/(name+'.png'))
def options():return lib.sm_state()==2 and word(0xde2)==3
reports=[]
for n,(seed,skill,progression) in enumerate([(14092026,'casual','medium'),(14092027,'regular','slow'),(14092028,'veteran','fast')]):
 assert lib.sm_seed_clear()
 draft=out/f'draft-{seed}.sram'
 draft.unlink(missing_ok=True)
 assert lib.sm_init(str(rom).encode(),str(draft).encode()),lib.sm_error()
 lib.sm_generation_configure(1,0)
 for i in range(9000):
  step(8 if i>180 and i%120<2 else 0)
  if options():break
 assert options(),(lib.sm_state(),word(0xde2))
 for _ in range(3):step()
 assert word(0x99e)==0 and not lib.sm_generation_take_request()
 press(8);assert options() and lib.sm_generation_state()==0, 'Start bypassed pending seed'
 # Controller Settings remains the original submenu; go down three rows.
 for _ in range(3):press(32)
 press(8)
 for _ in range(150):step()
 assert word(0xde2)==7,word(0xde2)
 while word(0x99e)!=7:
  press(32)
  for _ in range(25):step()
 press(8) # Exit, using the controller menu's original selection
 for _ in range(150):step()
 assert options(),(lib.sm_state(),word(0xde2))
 while word(0x99e)!=4:press(32)
 if n==0:capture('pending')
 press(8);assert lib.sm_generation_state()==1 and lib.sm_generation_take_request()==1
 assert lib.sm_generation_take_request()==0
 for _ in range(30):press(8);press(1)
 assert options() and lib.sm_generation_state()==1, 'Busy state let Start/Back escape'
 if n==0:capture('generating')
 # Retry is enabled after an explicit generation error.
 lib.sm_generation_fail(0);assert lib.sm_generation_state()==3
 if n==0:capture('error')
 press(8);assert lib.sm_generation_take_request()==1
 buffer=C.create_string_buffer(2097152)
 request=dict(seed=seed,skill=skill,progression=progression,patches=[])
 size=lib.sm_randomizer_generate(str(ROOT/'Randomizer').encode(),json.dumps(request).encode(),buffer,len(buffer))
 assert 0<size<=len(buffer),(size,buffer.value[:1000])
 envelope=json.loads(buffer.value);assert envelope['ok'],envelope
 manifest=envelope['manifest'];tracker=manifest['tracker'];placements=manifest['placements']
 assert tracker['seedFingerprint']==manifest['sha256'] and len(tracker['locations'])==100
 assert len({p['collectionBit'] for p in tracker['locations']})==100
 assert tracker['settings']['knows'] and tracker['settings']['preset']
 assert any(i['itemBits']==4 and i['type']=='Morph' for i in tracker['items']) and len(tracker['bosses'])==5
 assert all('openedBit' in d for d in tracker['topology']['doors'].values())
 assert tracker['topology']['doors'] and tracker['topology']['accessPoints'] and tracker['logicSources']
 assert manifest['solverVerification']['allItemsReachable'] and manifest['solverVerification']['motherBrainDefeated'] and manifest['solverVerification']['escapeToShip']
 assert hashlib.sha256(json.dumps(tracker,sort_keys=True,separators=(',',':')).encode()).hexdigest()==manifest['trackerSha256']
 items=(Item*100)(*(Item(p['address'],p['plm']) for p in placements))
 invalid=(Item*100)(*items);invalid[0].address=0
 save=out/manifest['sha256']/'sram.dat';save.parent.mkdir(exist_ok=True)
 assert not lib.sm_generation_commit(invalid,100,manifest['sha256'].encode(),str(save).encode())
 assert not lib.sm_generation_commit(items,100,manifest['sha256'].encode(),str(draft).encode())
 assert lib.sm_generation_commit(items,100,manifest['sha256'].encode(),str(save).encode())
 assert lib.sm_generation_state()==2 and options() and word(0x99e)==0
 assert [lib.sm_seed_rom_item(i) for i in range(100)]==[p['plm'] for p in placements]
 for _ in range(3):step()
 if n==0:capture('ready')
 for _ in range(4):press(32)
 press(8);assert options() and not lib.sm_generation_take_request(), 'Generate remained enabled'
 press(32);assert word(0x99e)==0
 press(8)
 for _ in range(3000):
  step()
  if lib.sm_state()==8:break
 assert lib.sm_state()==8 and lib.sm_room()==0x91f8,(lib.sm_state(),hex(lib.sm_room()))
 assert lib.sm_cpu_opcodes()==0
 # Capacity handshake rejects truncated buffers, snapshot cannot leak stale tail bytes.
 if n==0:
  for _ in range(450):step()
  assert lib.sm_test_seed_room(0)
  for _ in range(550):step()
  assert lib.sm_test_seed_pickup_position()
  for _ in range(45):step(64)
 snap=C.create_string_buffer(4096)
 lib.sm_tracker_snapshot.argtypes=[C.c_void_p,C.c_uint32]
 assert not lib.sm_tracker_snapshot(snap,4)
 assert lib.sm_tracker_snapshot(snap,len(snap))
 version=int.from_bytes(snap.raw[:4],'little');size=int.from_bytes(snap.raw[4:8],'little')
 assert version==1 and size==C.sizeof(Snapshot) and manifest['sha256'].encode() in snap.raw
 parsed=Snapshot.from_buffer_copy(snap.raw[:size]);assert parsed.randomized==1 and parsed.world_valid==1 and parsed.slot==0
 assert parsed.acquired_items==word(0x9a4) and parsed.acquired_beams==word(0x9a8)
 for i,loc in enumerate(tracker['locations']):
  bit=loc['collectionBit'];assert parsed.collected[i]==bool(parsed.item_bits[bit//8]&(1<<(bit%8)))
 if n==0:
  morph=next(i for i,p in enumerate(placements) if p['location']=='Morphing Ball')
  assert parsed.collected[morph]==1, 'Real pickup missing from tracker snapshot'
 again=Snapshot();assert lib.sm_tracker_snapshot(C.byref(again),C.sizeof(again)) and again.revision>parsed.revision and again.session==parsed.session
 lib.sm_shutdown();assert save.stat().st_size==8192
 (out/f'seed-{seed}.json').write_text(json.dumps(envelope,indent=2)+'\n')
 reports.append(dict(seed=seed,skill=skill,progression=progression,startLockedBefore=True,generationDisabledAfter=True,controllerSubmenuPreserved=True,
  retryAfterFailure=True,busyInputBlocked=True,all100NativePlacementsMatch=True,trackerLocations=100,
  snapshotVersion=version,snapshotSize=size,realPickupSnapshotVerified=n==0,nativeStartRoom='91f8',cpuOpcodes=0,solver=manifest['solverVerification']['solver']))
 print('PASS native Generate Game',seed,flush=True)
assert lib.sm_seed_clear()
# Vanilla keeps its original menu and Special Settings destination.
assert lib.sm_init(str(rom).encode(),str(out/'vanilla.sram').encode())
for i in range(9000):
 step(8 if i>180 and i%120<2 else 0)
 if options():break
assert options() and lib.sm_generation_state()==-1
assert (word(0x3000+(21*32+4)*2)&1023)==0x2b # Original SPECIAL SETTING MODE S
assert not word(0x3000+(6*32+4)*2)&0x400 # Start Game is enabled
for _ in range(4):press(32)
press(8)
for _ in range(150):step()
assert word(0xde2)==8
lib.sm_shutdown()
assert not (ROOT/'saves/sm.srm').exists(), 'Legacy shared SRAM write escaped the profile'
assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(out/'verification.json').write_text(json.dumps(dict(passed=True,host=platform.node(),nativeBinarySha256=hashlib.sha256((ROOT/'Native/build/libsm_native.so').read_bytes()).hexdigest(),cases=reports,sourceRomUnchanged=True,vanillaOptionsPreserved=True,legacySharedSramWriteRemoved=True,
 scope='Headless native core and in-process generator. Unreal UI profile persistence is not exercised by this test.'),indent=2)+'\n')
