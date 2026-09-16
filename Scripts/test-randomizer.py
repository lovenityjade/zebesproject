#!/usr/bin/env python3
"""Native ABI -> serialized spoiler -> decoded item table -> fresh solver.
Uses private buffers only; game ROM and SRAM must remain byte-identical.
"""
import ctypes
import hashlib
import json
from pathlib import Path
import sys
import time

ROOT=Path(__file__).resolve().parent.parent
OUT=ROOT/'Docs/Randomizer'
OUT.mkdir(parents=True,exist_ok=True)
(OUT/'verification.json').unlink(missing_ok=True)
lib=ctypes.CDLL(str(ROOT/'Native/build/libsm_native.so'))
lib.sm_randomizer_generate.argtypes=[ctypes.c_char_p,ctypes.c_char_p,ctypes.c_void_p,ctypes.c_int]
lib.sm_randomizer_error.restype=ctypes.c_char_p
ROM=ROOT/'roms/Super Metroid (Japan, USA) (En,Ja).sfc'
PROTECTED=[ROM,ROOT/'Unreal/Saved/SMPreview/sram.dat',ROOT/'Unreal/Saved/SM/sram.dat']
hashes={str(p):hashlib.sha256(p.read_bytes()).hexdigest() for p in PROTECTED if p.exists()}
rom=ROM.read_bytes()

def generate(request):
    buffer=ctypes.create_string_buffer(2097152)
    size=lib.sm_randomizer_generate(str(ROOT/'Randomizer').encode(),json.dumps(request).encode(),buffer,len(buffer))
    assert 0<size<=len(buffer),lib.sm_randomizer_error()
    return json.loads(buffer.value)

results=[]
started=time.monotonic()
for i in range(18):
    request=dict(seed=14092026+i,skill=('casual','regular','veteran')[i%3],progression=('slow','medium','fast')[(i//3)%3])
    output=generate(request)
    assert output['ok'],output
    manifest=output['manifest']
    verify=manifest['solverVerification']
    assert verify['difficulty']<=verify['maximumDifficulty']
    assert verify['allItemsReachable'] and verify['motherBrainDefeated'] and verify['escapeToShip'],verify
    # Round-trip through the actual 16-bit table format consumed by native PLMs.
    staged=bytearray(rom)
    for p in manifest['placements']:
        assert int.from_bytes(rom[p['address']:p['address']+2],'little') in range(0xeed7,0xefd0,4)
        staged[p['address']:p['address']+2]=p['plm'].to_bytes(2,'little')
    changed={n for n,(a,b) in enumerate(zip(rom,staged)) if a!=b}
    allowed={p['address']+offset for p in manifest['placements'] for offset in (0,1)}
    assert changed<=allowed
    for p in manifest['placements']:
        assert int.from_bytes(staged[p['address']:p['address']+2],'little')==p['plm']
    route=verify['progressionLog']
    assert all(any(p['location']==s['location'] and p['item']==s['item'] for p in manifest['placements'])
               for s in route if s['location'] in {p['location'] for p in manifest['placements']})
    (OUT/f'seed-{request["seed"]}.json').write_text(json.dumps(output,indent=2)+'\n')
    lines=[f'# Seed {request["seed"]} — {request["skill"]} / {request["progression"]}',
           '',f'100/100 objets accessibles ; Mother Brain et retour au vaisseau validés par le solver.',
           '', '| Étape | Lieu | Objet | Difficulté | Techniques |','|---:|---|---|---:|---|']
    lines += [f'| {s["step"]} | {s["location"]} | {s["item"]} | {s["difficulty"]} | {", ".join(s["techniques"])} |' for s in route]
    (OUT/f'progression-{request["seed"]}.md').write_text('\n'.join(lines)+'\n')
    results.append(dict(**request,sha256=manifest['sha256'],attempts=manifest['generationAttempts'],upstreamSeed=manifest['upstreamSeed'],reachable=verify['reachableItemCount'],difficulty=verify['difficulty'],steps=len(route),escape=True))
    print('PASS',request,'difficulty',verify['difficulty'],'steps',len(route),flush=True)
# Repeated requests after other skills/options cannot inherit mutable state.
first=generate(dict(seed=14092026,skill='casual',progression='slow'))
assert first['manifest']['sha256']==results[0]['sha256']
assert len({r['sha256'] for r in results})==len(results)
for request in ({'seed':0},{'seed':True},{'seed':-1},{'seed':1,'skill':'../casual'},
                {'seed':1,'patches':['unknown']},{'seed':1,'bossRandomization':True},
                {'seed':1,'patches':['fast_doors','fast_doors']}):
    assert not generate(request)['ok'],request
assert generate(dict(seed=14092026,patches=['max_ammo_display','refill_before_save']))['manifest']['pendingPatches']==['max_ammo_display','refill_before_save']
assert all(hashlib.sha256(Path(p).read_bytes()).hexdigest()==digest for p,digest in hashes.items())
report=dict(passed=True,seeds=results,reproducibility=True,invalidRequestsRejected=7,
            romAndSramUnchanged=True,romSha1=hashlib.sha1(rom).hexdigest(),
            elapsedSeconds=round(time.monotonic()-started,2),
            scope='Integrated native ABI, serialization, item-table round trip, logical completion and escape; not a native gameplay playthrough.')
(OUT/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('SM_RANDOMIZER_SEEDS_PASS',flush=True)
