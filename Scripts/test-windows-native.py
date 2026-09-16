#!/usr/bin/env python3
"""Windows native ABI, embedded services and UTF-8 save regression.

Run with the bundled interpreter in an explicitly isolated build tree. No user
profile is opened. The external C host proves Python/MSYS2 are not PATH needs.
"""
import ctypes as C
import hashlib
import json
import os
from pathlib import Path
import re
import shutil
import subprocess
import sys

root = Path(sys.argv[1]).resolve()
assert sys.platform == 'win32' and (root / 'ISOLATED_TEST_DIRECTORY').is_file()
assert sys.flags.isolated, 'Use Runtime/Python/python.exe -I'
out = root / 'SMTests' / 'Windows port é 漢字'
out.mkdir(parents=True, exist_ok=True)
os.chdir(out)
# No developer DLLs or Python environment may satisfy a missing runtime.
env = {k: v for k, v in os.environ.items() if not k.upper().startswith('PYTHON')}
env['PATH'] = str(Path(os.environ['SystemRoot']) / 'System32')
os.environ.clear()
os.environ.update(env)
lib = C.CDLL(str(root / 'Native/build/sm_native.dll'))
exports = set()
for header in (root / 'Native').glob('*.h'):
    exports.update(re.findall(r'SM_API\s+[^;{]+?\b(sm_\w+)\s*\(', header.read_text(encoding='utf-8')))
for symbol in exports:
    getattr(lib, symbol)
lib.sm_error.restype = C.c_char_p
lib.sm_randomizer_error.restype = C.c_char_p
lib.sm_init.argtypes = [C.c_char_p, C.c_char_p]
lib.sm_cpu_opcodes.restype = C.c_uint64
lib.sm_simulation_ram.restype = C.c_void_p
lib.sm_profile_us.argtypes = [C.c_int]
lib.sm_profile_us.restype = C.c_double

def embedded(service, request):
    fn = getattr(lib, 'sm_' + service)
    fn.argtypes = [C.c_char_p, C.c_char_p, C.c_void_p, C.c_int]
    buffer = C.create_string_buffer(2097152)
    size = fn(str(root / 'Randomizer').encode(), json.dumps(request).encode(), buffer, len(buffer))
    assert 0 < size <= len(buffer), lib.sm_randomizer_error()
    return json.loads(buffer.value)

def external(request, service=None):
    command = [str(root / 'Native/build/sm_randomizer_host.exe'), str(root / 'Randomizer'), json.dumps(request)]
    if service:
        command.append(service)
    result = subprocess.run(command, env=env, capture_output=True, check=True, timeout=240)
    return json.loads(result.stdout)

assert external({'seed': 0}, 'validate')['ok']
assert not embedded('randomizer_validate', {'seed': -1})['ok']
assert not embedded('randomizer_validate', {'seed': 0, 'patches': ['unknown']})['ok']
assert embedded('randomizer_validate', {'seed': 0})['ok']
reports = []
first = None
for i, skill in enumerate(('casual', 'regular', 'veteran')):
    request = dict(seed=14092026+i, skill=skill, progression='slow')
    response = external(request) if i == 0 else embedded('randomizer_generate', request)
    assert response['ok'], response
    manifest = response['manifest']
    verify = manifest['solverVerification']
    assert verify['allItemsReachable'] and verify['completionVerified'] and verify['escapeToShip'], verify
    assert verify['difficulty'] <= verify['maximumDifficulty']
    assert len(manifest['placements']) == 100
    placements = {p['location']: p['item'] for p in manifest['placements']}
    for step in verify['progressionLog']:
        if step['location'] in placements:
            assert placements[step['location']] == step['item']
    (out / f'seed-{request["seed"]}.json').write_text(json.dumps(response, indent=2), encoding='utf-8')
    tracker = manifest['tracker']
    query = dict(randomized=True, settings=tracker['settings'], topology=tracker['topology'],
                 inventory=dict(items=0, beams=0, health=99, reserve=0, missiles=0, supers=0,
                                powerBombs=0, bosses=[0]*8, doors=[0]*64, collected=[0]*100))
    empty = embedded('tracker_evaluate', query)
    assert empty['ok'] and len(empty['checks']) == 100 and len(empty['bosses']) == 10, empty
    query['inventory'].update(items=0xf32f, beams=0x100f, health=1499, reserve=400, missiles=230, supers=50, powerBombs=50)
    full = embedded('tracker_evaluate', query)
    assert full['ok'] and sum(c['state'] == 1 for c in full['checks']) > sum(c['state'] == 1 for c in empty['checks'])
    reports.append(dict(seed=request['seed'], fingerprint=manifest['sha256'], checks=100,
                        progressionSteps=len(verify['progressionLog']), completionVerified=True))
    if first is None:
        first = manifest['sha256']
    print('PASS generation / progression / tracker:', request['seed'], flush=True)
again = embedded('randomizer_generate', dict(seed=14092026, skill='casual', progression='slow'))
assert again['ok'] and again['manifest']['sha256'] == first, 'Request state leaked between seeds'

original = next((root / 'roms').glob('*.sfc'))
original_hash = hashlib.sha256(original.read_bytes()).hexdigest()
rom = out / 'Métroïde.sfc'
shutil.copy2(original, rom)
save = out / 'sauvegarde.dat'
assert not lib.sm_init(str(out / 'missing.sfc').encode(), str(save).encode())
bad = out / 'invalid.sfc'
data = bytearray(rom.read_bytes()); data[0] ^= 1; bad.write_bytes(data)
assert not lib.sm_init(str(bad).encode(), str(save).encode())
assert b'CRC32' in lib.sm_error()
assert not save.exists(), 'ROM gate wrote a save'
assert lib.sm_init(str(rom).encode(), str(save).encode()), lib.sm_error()
for i in range(360):
    assert lib.sm_step(0), lib.sm_error()
assert lib.sm_cpu_opcodes() == 0
assert 0 < lib.sm_profile_us(0) < 10000000
assert lib.sm_save(), lib.sm_error()
assert save.stat().st_size == 8192
# Read the SRAM through the public ABI and verify it survives a second commit.
sram = (C.c_uint8 * 8192)()
lib.sm_slots_copy_sram.argtypes = [C.c_void_p, C.c_int]
assert lib.sm_slots_copy_sram(sram, len(sram)) == 8192
assert lib.sm_save(), 'Second save must replace an existing file on Windows'
before = save.read_bytes()
lib.sm_shutdown()
assert lib.sm_init(str(rom).encode(), str(save).encode()), lib.sm_error()
assert lib.sm_slots_copy_sram(sram, len(sram)) == 8192
assert bytes(sram) == before
lib.sm_shutdown()
assert hashlib.sha256(original.read_bytes()).hexdigest() == original_hash
assert hashlib.sha256(rom.read_bytes()).hexdigest() == original_hash
report = dict(platform='Win64', exports=len(exports), seeds=reports,
              isolatedBundledPython=True, noDeveloperPath=True, unicodeRomAndSave=True,
              replaceExistingSave=True, missingAndInvalidRomBlocked=True, nativeOpcodes=0,
              renderedGameTested=False)
(out.parent / 'windows-native-verification.json').write_text(json.dumps(report, indent=2), encoding='utf-8')
print(json.dumps(report, indent=2), flush=True)
