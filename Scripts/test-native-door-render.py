#!/usr/bin/env python3
"""Load patched common room graphics and render native beam doors, gaming-pc."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-door-colors.py').read_text().split('cases=[]')[0]
source=source.replace("root/'door-colors/native'","root/'door-colors/render'")
exec(compile(source,'door-render-fixture','exec'))
values=(C.c_uint8*58)(*[5 if d['canRandom'] else 0 for d in locations])
assert l.sm_doors_configure(0,values,58,catalog['sha256'].encode());activate(0);assert select(items,100,m['sha256'].encode())
spawn=routine('SpawnRoomPLM',None,C.c_uint16);handler=routine('PlmHandler_Async',C.c_int);results=[]
for record in catalog['records']:
 if record['indicator']:continue
 assert l.sm_test_all_equipment()
 assert l.sm_test_room(0x948c,96,104)
 for _ in range(2000):
  step()
  if l.sm_state()==8 and l.sm_room()==0x948c:break
 assert l.sm_state()==8 and l.sm_room()==0x948c
 C.memset(ram+0x1c37,0,80);C.memset(ram+0xd8b0,0,64)
 entry=record['plm'].to_bytes(2,'little')+bytes([10,6,5,0]);C.memmove(ram+0x12,entry,6);spawn(0x12)
 wait(20)
 assert word(0xde6c+78)==record['draw']
 label=record['color']+'-'+str(record['direction']);capture(label+'-closed')
 put(0x1d77+78,{'wave':1,'ice':2,'spazer':4,'plasma':8}[record['color']]);assert handler()==0
 assert read(0xd8b0,1)[0]&32,(record,hex(word(0x1cd7+78)),hex(word(0x1d27+78)),hex(word(0x1c37+78)))
 wait(120)
 assert read(0xd8b0,1)[0]&32 and word(0x1c37+78)==0,(record,l.sm_state(),hex(word(0x1c37+78)),hex(word(0x1d27+78)))
 capture(label+'-open');results.append(dict(color=record['color'],direction=record['direction'],room=l.sm_room()))
 assert l.sm_cpu_opcodes()==0
 print('DOOR_RENDER_PASS',label,flush=True)
l.sm_shutdown();assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(out/'verification.json').write_text(json.dumps(dict(plms=results,cpu=0,sourceRomUnchanged=True,scope='Actual native room load, graphics decompression and frames; deliberate PLM insertion at block 10,6, not generated placement or a traversal'),indent=2))
