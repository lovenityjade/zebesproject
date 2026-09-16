#!/usr/bin/env python3
"""Controlled native load/save fixtures for all pinned starts, gaming-pc only.
Uses one existing item plan to test room behavior; not seed-route validation.
"""
import sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
source=(root/'test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
source=source.replace("root/'Native/build/libsm_native.so'","root/'world-data/libsm_native.so'").replace("out=root/'tracker-results'","out=root/'world-data/start-probes'")
source=source.replace("('plm',C.c_uint16)]", "('plm',C.c_uint16),('kind',C.c_uint16)]").replace("Item(i['address'],i['plm'])", "Item(i['address'],i['plm'],i.get('kind',0))")
needle=' put(0x79f,0);put(0x78b,0);put(0xd914,5);put(0x998,6)'
assert needle in source;source=source.replace(needle,' prepare_start()')
exec(compile(source,'start-fixture','exec'))
import subprocess,uuid
symbols={p[-1]:int(p[0],16) for line in subprocess.check_output(['nm','-an',str(root/'world-data/libsm_native.so')],text=True).splitlines() if len(p:=line.split())==3 and all(c in '0123456789abcdefABCDEF' for c in p[0])}
base=C.cast(l.sm_init,C.c_void_p).value-symbols['sm_init']
def routine(name):return C.CFUNCTYPE(None)(base+symbols[name])
activate=C.CFUNCTYPE(C.c_int,C.c_int,C.c_int)(base+symbols['sm_start_activate'])
select=C.CFUNCTYPE(C.c_int,C.POINTER(Item),C.c_int,C.c_char_p)(base+symbols['sm_seed_select_plan'])
save=C.CFUNCTYPE(None,C.c_uint16)(base+symbols['SaveToSram']);reload=C.CFUNCTYPE(C.c_uint8,C.c_uint16)(base+symbols['LoadFromSram'])
l.sm_start_configure.argtypes=[C.c_int,C.c_int,C.POINTER(C.c_uint8),C.c_int,C.c_char_p]
catalog=json.loads((root/'Randomizer/native_world_data.json').read_text());audit=json.loads((root/'world-data/dependencies.json').read_text());ids={p['name']:p['id'] for p in catalog['patches']}
def prepare_start():
 global expected_room
 deps=[]
 for name in target['directPatches']+sum(target['conditionalGroups'].values(),[]):
  if name in ids and ids[name] not in deps:deps.append(ids[name])
 patches=(C.c_uint8*len(deps))(*deps)
 assert l.sm_start_configure(0,target['start']['spawn'],patches,len(deps),catalog['sha256'].encode())
 assert activate(0,1)
 items=(Item*100)(*[Item(i['address'],i['plm'],i.get('kind',0)) for i in m['placements']]);assert select(items,100,m['sha256'].encode())
 routine('sm_start_new_game')()
 romptr=C.c_void_p.from_address(base+symbols['g_rom']).value
 table=int.from_bytes(C.string_at(romptr+0x44b5+word(0x79f)*2,2),'little')
 entry=(table&0x7fff)+14*word(0x78b);expected_room=int.from_bytes(C.string_at(romptr+entry,2),'little')
results=[]
for target in audit['starts']:
 print('START_LOADING',target['name'],flush=True)
 boot('start-'+uuid.uuid4().hex,True)
 assert word(0x79b)==expected_room,(target['name'],hex(word(0x79b)),hex(expected_room))
 spawn=target['start']['spawn'];area=6 if spawn==65534 else spawn>>8;station=0 if spawn==65534 else spawn&255
 assert word(0x79f)==area and word(0x78b)==station
 for door in target['start'].get('doors',[]):assert read(0xd8b0+door//8,1)[0]&(1<<(door%8)),(target['name'],'door',door)
 if spawn and spawn!=65534:
  assert read(0xd820,1)[0]&1
  assert not read(0xd8b0+50//8,1)[0]&(1<<(50%8)), 'Inherited Landing Site door'
 if key:=target['start'].get('save'):
  plm=audit['patches'][key]['additionalPlm']['plm_bytes_list'][0];block=2*(plm[3]*word(0x7a5)+plm[2])
  stations=[i for i in range(40) if word(0x1c37+i*2)==0xb76f and word(0x1c87+i*2)==block]
  assert stations,(target['name'],'missing custom save')
  # Exact native PLM argument must be the new load record's station index.
  assert word(0x1dc7+stations[0]*2)==station
  routine('sm_start_room')();assert len([i for i in range(40) if word(0x1c37+i*2)==0xb76f and word(0x1c87+i*2)==block])==1
 save(0);put(0x79f,0);put(0x78b,0);assert reload(0)==0;assert word(0x79f)==area and word(0x78b)==station
 capture(target['name'].replace(' ','-'))
 results.append(dict(start=target['name'],room=word(0x79b),area=area,station=station,customSave=bool(target['start'].get('save')),health=word(0x9c2)))
 assert l.sm_cpu_opcodes()==0;l.sm_shutdown();print('NATIVE_START_PASS',target['name'],flush=True)
assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(out/'verification.json').write_text(json.dumps(dict(starts=results,cpu=0,sourceRomUnchanged=True,scope='Native load and SRAM round-trip, controlled item plan; not randomized progression'),indent=2))
