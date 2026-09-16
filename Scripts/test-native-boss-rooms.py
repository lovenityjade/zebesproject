#!/usr/bin/env python3
"""Controlled real room loads for every directed boss pairing, gaming-pc."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-tweaks.py').read_text().split('for slot,names in enumerate')[0]
source=source.replace("(root/'tweaks').mkdir(exist_ok=True)","(root/'connections').mkdir(exist_ok=True)").replace("root/'tweaks/native'","root/'connections/rooms'").replace('world-data/libsm_native.so','connections/libsm_native.so')
exec(compile(source,'connection-room-fixture','exec'))
import itertools
catalog=json.loads((root/'Randomizer/native_connections.json').read_text());aps=catalog['accessPoints']
inside=[a['id'] for a in aps if a['inside']];outside=[a['id'] for a in aps if not a['inside']]
l.sm_connections_configure.argtypes=[C.c_int,C.POINTER(C.c_uint8),C.c_int,C.c_char_p]
activate=routine('sm_connections_activate',None,C.c_int);seen=set();results=[]
for permutation in itertools.permutations(inside):
 mapping=[0]*8
 for a,b in zip(outside,permutation):mapping[a]=b;mapping[b]=a
 assert l.sm_connections_configure(0,(C.c_uint8*8)(*mapping),8,catalog['sha256'].encode());activate(0)
 assert select(items,100,m['sha256'].encode())
 for i,j in enumerate(mapping):
  if (i,j) in seen:continue
  seen.add((i,j));assert l.sm_test_all_equipment()
  assert l.sm_test_boss_connection(i)
  for _ in range(2000):
   step()
   if l.sm_state()==8:break
  assert l.sm_state()==8 and l.sm_room()==aps[j]['room']['RoomPtr'],(aps[i]['name'],aps[j]['name'],l.sm_state(),hex(l.sm_room()))
  wait(120)
  assert l.sm_state()==8 and word(0x9c2)>0 and word(0x79f)==aps[j]['room']['area']
  x,y=word(0xaf6),word(0xafa)
  assert x<word(0x7a5)*16 and y<word(0x7a7)*16,(aps[j]['name'],x,y)
  capture(aps[i]['name']+'--'+aps[j]['name'])
  results.append(dict(source=aps[i]['name'],destination=aps[j]['name'],room=l.sm_room(),area=word(0x79f),x=x,y=y,health=word(0x9c2)))
  assert l.sm_cpu_opcodes()==0;print('BOSS_ROOM_PASS',len(results),aps[i]['name'],aps[j]['name'],flush=True)
assert len(results)==32;l.sm_shutdown();assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(out/'verification.json').write_text(json.dumps(dict(transitions=results,cpu=0,sourceRomUnchanged=True,scope='Native debug room loads with all-equipment fixture, not manual source-door crossing or generated boss routes'),indent=2))
