#!/usr/bin/env python3
"""Initial-door contract and legacy isolation through native save initialization.

Controlled RAM fixtures on gaming-pc; persistence is covered by the slot suite.
"""
import os, sys
from pathlib import Path
root = Path(sys.argv[1]).resolve()
assert os.uname().nodename == 'gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source = (root/'test-native-area-connections.py').read_text().split('cases=[];visited=set()')[0]
source = source.replace("root/'areas/native'", "root/'areas/initial-doors'")
exec(compile(source, 'initial-door-fixture', 'exec'))
sys.path.insert(0, str(root/'Randomizer/upstream'))
from logic.logic import Logic
Logic.factory('vanilla')
from graph.graph_utils import getAccessPoint
from utils.doorsmanager import DoorsManager

initial = json.loads((root/'Randomizer/native_initial_doors.json').read_text())
world = json.loads((root/'Randomizer/native_world_data.json').read_text())
ids = {p['name']: p['id'] for p in world['patches']}
native_init = routine('sm_start_initialize_save', None)
initial_configure = l.sm_start_configure_initial_doors
initial_configure.argtypes = [C.c_int, C.c_int, C.POINTER(C.c_uint8), C.c_int]

def config_initial(slot, spawn, opened):
    return initial_configure(slot, spawn, (C.c_uint8*len(opened))(*opened), len(opened))

def config_start(slot, entry):
    info = getAccessPoint(entry['name']).Start
    patches = [ids[info['save']]] if 'save' in info else []
    return l.sm_start_configure(slot, entry['spawn'], (C.c_uint8*len(patches))(*patches), len(patches), world['sha256'].encode())

def initialized_bits():
    C.memmove(ram, baseline, len(baseline))
    C.memset(ram+0xd8b0, 0, 64)
    native_init()
    return [bit for bit in range(512) if read(0xd8b0+bit//8, 1)[0] & (1 << (bit%8))]

base_doors = [0x10]
DoorsManager.getBlueDoors(base_doors)
assert initial['base'] == sorted(set(base_doors))
cases = []
for index, entry in enumerate(initial['starts']):
    slot = index%4
    info = getAccessPoint(entry['name']).Start
    expected = sorted(set(base_doors+info.get('doors', [])))
    assert expected == entry['opened']
    assert configure(slot, [])
    if entry['areaOnly']:
        assert not config_start(slot, entry)
    assert configure(slot, [(1-i)%32 for i in range(32)])
    assert config_start(slot, entry)
    assert config_initial(slot, entry['spawn'], expected)
    assert activate(slot, 1)
    assert initialized_bits() == expected, entry['name']
    assert word(0x79f) == (6 if entry['spawn'] == 65534 else entry['spawn'] >> 8)
    assert word(0x78b) == (0 if entry['spawn'] == 65534 else entry['spawn'] & 255)
    # Invalid replacements must retain the previously accepted complete list.
    invalids = [expected[:-1], expected+[255], [255]+expected[1:], [expected[1]]+expected[1:]]
    for invalid in invalids:
        assert not config_initial(slot, entry['spawn'], invalid)
        assert activate(slot, 1) and initialized_bits() == expected
    assert not initial_configure(slot, entry['spawn'], None, len(expected))
    assert not initial_configure(slot, entry['spawn'], None, -1)
    assert not config_initial(slot, 12345, expected)
    if entry['areaOnly']:
        assert configure(slot, [])
        assert not activate(slot, 1)
        assert configure(slot, [(1-i)%32 for i in range(32)])
        assert activate(slot, 1) and initialized_bits() == expected
    cases.append(dict(name=entry['name'],slot=slot,spawn=entry['spawn'],opened=expected,invalidReplacements=len(invalids)))

# Four slots preserve independent lists, including an explicit empty contract
# for historical seeds. Legacy initializers must not gain new refill-door bits.
entries = [initial['starts'][1], initial['starts'][3], initial['starts'][13], initial['starts'][1]]
for slot, entry in enumerate(entries):
    assert configure(slot, [(1-i)%32 for i in range(32)])
    assert config_start(slot, entry)
    assert config_initial(slot, entry['spawn'], entry['opened'] if slot != 3 else [])
for slot in [0,1,2,3,1,0,3,2]:
    assert activate(slot, 1)
    assert initialized_bits() == (entries[slot]['opened'] if slot != 3 else [50])

# A complete list for a different start cannot be activated accidentally.
assert config_initial(0, entries[1]['spawn'], entries[1]['opened'])
assert not activate(0, 1)
assert config_initial(0, entries[0]['spawn'], entries[0]['opened'])
assert activate(0, 1) and initialized_bits() == entries[0]['opened']
assert not config_initial(-1, 0, entries[0]['opened'])
assert not config_initial(4, 0, entries[0]['opened'])
assert l.sm_cpu_opcodes() == 0
l.sm_shutdown()
assert hashlib.sha256(rom.read_bytes()).hexdigest() == rom_hash
(out/'verification.json').write_text(json.dumps(dict(passed=True, starts=cases, independentSelectors=4,
    legacyEmptyContract=True, mismatchedSpawnRejected=True, areaOnlyDependencyGuard=True, cpu=0,
    sourceRomUnchanged=True, nativeSha256=hashlib.sha256(libpath.read_bytes()).hexdigest(), scope=__doc__), indent=2)+'\n')
print('INITIAL_DOORS_PASS', len(cases), 'starts', flush=True)
