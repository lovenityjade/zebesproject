#!/usr/bin/env python3
import ctypes as C,json,hashlib,tempfile
from pathlib import Path
ROOT=Path(__file__).resolve().parent.parent
lib=C.CDLL(str(ROOT/'Native/build/libsm_native.so'))
class Item(C.Structure):_fields_=[('address',C.c_uint32),('plm',C.c_uint16)]
lib.sm_seed_stage.argtypes=[C.POINTER(Item),C.c_int,C.c_char_p]
lib.sm_init.argtypes=[C.c_char_p,C.c_char_p]
lib.sm_error.restype=C.c_char_p
rom=ROOT/'roms/Super Metroid (Japan, USA) (En,Ja).sfc';rom_hash=hashlib.sha256(rom.read_bytes()).hexdigest()
results=[]
with tempfile.TemporaryDirectory(prefix='sm-native-seeds-') as tmp:
 for seed in (14092026,14092027,14092028,14092029,14092030,14092031,14092032,14092040):
    m=json.loads((ROOT/f'Docs/Randomizer/seed-{seed}.json').read_text())['manifest']
    placements=sorted(m['placements'],key=lambda p:p['address'])
    items=(Item*100)(*(Item(p['address'],p['plm']) for p in placements))
    assert lib.sm_seed_clear()
    invalid=(Item*100)(*items);invalid[0].address=0
    assert not lib.sm_seed_stage(invalid,100,m['sha256'].encode())
    assert lib.sm_seed_stage(items,100,m['sha256'].encode())
    # Vanilla saves cannot accidentally be used for a randomizer session.
    assert not lib.sm_init(str(rom).encode(),str(Path(tmp)/'vanilla.sram').encode())
    save=Path(tmp)/f'{m["sha256"]}.sram'
    assert lib.sm_init(str(rom).encode(),str(save).encode()),lib.sm_error()
    assert [lib.sm_seed_rom_item(i) for i in range(100)]==[p['plm'] for p in placements]
    assert not lib.sm_seed_stage(items,100,m['sha256'].encode())
    assert not lib.sm_seed_clear()
    for _ in range(8500):
        f,s=lib.sm_frame(),lib.sm_state()
        assert lib.sm_step(8 if f>180 and f%120<2 and not 7<=s<=18 else 0),lib.sm_error()
        if lib.sm_state()==8:break
    assert lib.sm_state()==8 and lib.sm_room()==0x91f8,(lib.sm_state(),hex(lib.sm_room()))
    for _ in range(450):assert lib.sm_step(0),lib.sm_error()
    assert lib.sm_test_seed_room(seed%2)
    for _ in range(550):assert lib.sm_step(0),lib.sm_error()
    assert lib.sm_room()==0x9e9f
    index=next(i for i,p in enumerate(placements) if p['location']=='Morphing Ball')
    assert not lib.sm_seed_location_collected(index)
    before=[lib.sm_seed_inventory(i) for i in range(7)]
    assert lib.sm_test_seed_pickup_position()
    for _ in range(45):assert lib.sm_step(64),lib.sm_error()
    assert lib.sm_seed_location_collected(index),('No native pickup',seed)
    after=[lib.sm_seed_inventory(i) for i in range(7)]
    item=placements[index]['item']
    expected=before.copy()
    if item=='Missile':expected[1]+=5
    elif item=='Morph':expected[4]|=4
    elif item=='HiJump':expected[4]|=0x100
    elif item=='PowerBomb':expected[3]+=5
    else:raise AssertionError(('Unspecified inventory expectation',item))
    assert after==expected,(seed,item,before,after,expected)
    assert lib.sm_message_active(),('Missing native item message',seed)
    assert lib.sm_cpu_opcodes()==0
    frame=lib.sm_frame();lib.sm_shutdown()
    assert save.exists() and save.stat().st_size==8192
    results.append(dict(seed=seed,itemsReadBack=100,startRoom='91f8',frame=frame,cpuOpcodes=0,realPickup=item,awakened=bool(seed%2),locationBitConfirmed=True,inventoryBefore=before,inventoryAfter=after))
    print('PASS native seed',seed,flush=True)
 assert lib.sm_seed_clear()
 # Restoring vanilla loads the original table again, not the previous seed.
 assert lib.sm_init(str(rom).encode(),str(Path(tmp)/'vanilla.sram').encode())
 expected=[int.from_bytes(rom.read_bytes()[p['address']:p['address']+2],'little') for p in placements]
 assert [lib.sm_seed_rom_item(i) for i in range(100)]==expected
 lib.sm_shutdown()
assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
report=dict(passed=True,seeds=results,invalidAddressesRejected=True,vanillaSaveRejected=True,
            liveMutationRejected=True,vanillaTableRestored=True,sourceRomUnchanged=True,
            scope='Real C-only boots, 100 item words read back, normal PLM collision pickups from a positioned test fixture in sleeping/awakened Morph room, matching inventory and message; complete progression is verified separately by the solver.')
(ROOT/'Docs/Randomizer/native-verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('SM_SEED_NATIVE_PASS')
