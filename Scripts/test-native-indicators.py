#!/usr/bin/env python3
"""Native PLM lifecycle and world transactions; gaming-pc only."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-tweaks.py').read_text().split('for slot,names in enumerate')[0]
source=source.replace("(root/'tweaks').mkdir(exist_ok=True)","(root/'indicators').mkdir(exist_ok=True)").replace("root/'tweaks/native'","root/'indicators/native'")
exec(compile(source,'indicator-fixture','exec'))
class Indicator(C.Structure):_fields_=[('location',C.c_uint16),('plm',C.c_uint16)]
l.sm_start_configure_world.argtypes=[C.c_int,C.c_int,C.POINTER(C.c_uint8),C.c_int,C.c_char_p,C.POINTER(Indicator),C.c_int]
definition=next(p for p in catalog['patches'] if p['name']=='door_indicators_plms.ips')
locations=definition['indicatorLocations'];patch=(C.c_uint8*1)(definition['id'])
entries=(Indicator*len(locations))(*[Indicator(i,v['vanillaPlm']) for i,v in enumerate(locations)])
def configure(slot,entries):return l.sm_start_configure_world(slot,0,patch,1,catalog['sha256'].encode(),entries,len(entries))
assert configure(0,entries)
for bad in [[Indicator(18,0xfbb0)],[Indicator(0,0xfbb0)],[Indicator(0,0xfbb6),Indicator(0,0xfbb6)],[Indicator(0,0xf611)]]:
 assert not configure(0,(Indicator*len(bad))(*bad))
assert not l.sm_start_configure_world(1,0,None,0,catalog['sha256'].encode(),entries,len(entries))
assert activate(0,1);assert select(items,100,m['sha256'].encode())
romptr=C.c_void_p.from_address(base+symbols['g_rom']).value
replacements={int(a):bytes(b) for x in locations if x['romReplacements'] for a,b in x['romReplacements'].items()}
for i,loc in enumerate(locations):
 if loc['romReplacements']:
  a,template=next(iter(loc['romReplacements'].items()));expected=entries[i].plm.to_bytes(2,'little')+bytes(template[2:])
  assert C.string_at(romptr+int(a),6)==expected
 else:
  C.memmove(ram,baseline,len(baseline));C.memset(ram+0x1c37,0,80)
  put(0x79b,loc['additionalPlms']['room']);put(0x7a5,128)
  entry,=loc['additionalPlms']['plm_bytes_list'];block=2*(entry[3]*128+entry[2])
  routine('sm_indicators_room',None)()
  found=[j for j in range(40) if word(0x1c37+2*j)==entries[i].plm and word(0x1c87+2*j)==block]
  assert len(found)==1 and word(0x1dc7+2*found[0])==int.from_bytes(bytes(entry[4:]),'little')
  routine('sm_indicators_room',None)();assert sum(word(0x1c37+2*j)==entries[i].plm for j in range(40))==1
assert configure(1,(Indicator*0)())
assert activate(1,1)
# Merely preparing slot 1 leaves slot 0's committed room selection intact.
first=locations[0];put(0x79b,first['additionalPlms']['room']);C.memset(ram+0x1c37,0,80)
routine('sm_indicators_room',None)();assert word(0x1c37+78)==entries[0].plm
assert select(items,100,m['sha256'].encode());C.memset(ram+0x1c37,0,80)
routine('sm_indicators_room',None)();assert word(0x1c37+78)==0
assert activate(0,1);assert select(items,100,m['sha256'].encode())
assert select(None,0,None)
source_rom=rom.read_bytes()
for a in replacements:assert C.string_at(romptr+a,6)==source_rom[a:a+6]
assert select(items,100,m['sha256'].encode())
spawn=routine('SpawnRoomPLM',None,C.c_uint16);handler=routine('PlmHandler_Async',C.c_int)
draws=[]
for color in range(4):
 for direction in range(4):
  C.memmove(ram,baseline,len(baseline));C.memset(ram+0x1c37,0,80);C.memset(ram+0xd8b0,0,64)
  plm=0xfbb0+(color*4+direction)*6
  entry=plm.to_bytes(2,'little')+bytes([6,6,5,0]);C.memmove(ram+0x12,entry,6);spawn(0x12)
  index=39;put(0x1c23,0x8000);blue=0xa9b3+direction*0x3c;colored=[0xa8e7,0xa827,0xa767,0xa6a7][color]+direction*0x30
  # Invoke the actual incoming-door cap lookup; temporarily supply matching
  # coordinates in a native ROM door record, then restore that record at once.
  put(0x78d,0x8ad2);dd=romptr+0x18ad2;before=C.string_at(dd,12)
  replacement=bytearray(before);replacement[4:6]=bytes([6,6]);C.memmove(dd,bytes(replacement),12)
  try:assert routine('CheckIfColoredDoorCapSpawned',C.c_uint8)()==1
  finally:C.memmove(dd,before,12)
  closed=[]
  for frame in range(8):
   assert handler()==0;closed.append(word(0xde6c+index*2))
  assert closed==[0xa677+direction*12]*2+[0xa9d7+direction*0x3c]*2+[0xa9cb+direction*0x3c]*2+[0xa9bf+direction*0x3c]*2
  phases=[]
  for frame in range(19):
   assert handler()==0
   phases.append(word(0xde6c+index*2))
  assert phases==[blue]*9+[colored]*9+[blue],(color,direction,phases)
  # X-ray must not treat these high-address PLMs as item definitions.
  routine('LoadXrayBlocks',None)()
  put(0x1d77+index*2,0x100);assert handler()==0
  assert word(0x1d77+index*2)==0 and not (read(0xd8b0,1)[0]&32), 'Indicator set opposite door bit'
  # Re-enter after the opposite door was opened: original blue door list.
  C.memmove(ram+0xd8b0,bytes([32]),1);C.memset(ram+0x1c37,0,80);C.memmove(ram+0x12,entry,6);spawn(0x12)
  assert handler()==0 and word(0xde6c+index*2)==blue
  assert word(0x1d27+index*2)==0xc4b1+direction*0x31+7
  assert handler()==0 and word(0x1c37+index*2)==0
  draws.append(dict(plm=plm,blue=blue,colored=colored,closingFrames=closed,frames=phases))
routine('SaveToSram',None,C.c_uint16)(0)
C.memset(ram+0xd8b0,0,64)
assert routine('LoadFromSram',C.c_uint8,C.c_uint16)(0)==0
assert read(0xd8b0,1)[0]&32
assert l.sm_cpu_opcodes()==0;l.sm_shutdown();assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(out/'verification.json').write_text(json.dumps(dict(locations=len(locations),plms=draws,invalidRejected=5,vanillaRestored=True,xraySafe=True,slotIsolation=True,openedBitSaveReload=True,cpu=0,nativeSha256=hashlib.sha256(libpath.read_bytes()).hexdigest()),indent=2))
print('NATIVE_INDICATORS_PASS',len(locations),len(draws),flush=True)
