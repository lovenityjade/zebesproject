#!/usr/bin/env python3
"""Isolated native colored-door data/routines; gaming-pc only."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-tweaks.py').read_text().split('for slot,names in enumerate')[0]
source=source.replace("(root/'tweaks').mkdir(exist_ok=True)","(root/'door-colors').mkdir(exist_ok=True)").replace("root/'tweaks/native'","root/'door-colors/native'").replace('world-data/libsm_native.so','door-colors/libsm_native.so')
exec(compile(source,'door-colors-fixture','exec'))
import itertools
sys.path.insert(0,str(root/'Randomizer/upstream'))
from rom.ips import IPS_Patch
catalog=json.loads((root/'Randomizer/native_door_colors.json').read_text());locations=catalog['locations'];colors=catalog['colors']
assert len(locations)==58
l.sm_doors_configure.argtypes=[C.c_int,C.POINTER(C.c_uint8),C.c_int,C.c_char_p]
activate=routine('sm_doors_activate',None,C.c_int);check=routine('CallPlmPreInstr',None,C.c_uint32,C.c_uint16)
initial=routine('sm_doors_initialize_save',None)
romptr=C.c_void_p.from_address(base+symbols['g_rom']).value
assert select(None,0,None);original_runtime=C.string_at(romptr,0x300000)
patch_data={}
for name,definition in catalog['patches'].items():
 patch=IPS_Patch.load(str(root/f'Randomizer/upstream/patches/common/ips/{name}.ips')).toDict()
 for address,values in patch.items():
  for i,value in enumerate(values):
   a=address+i
   if not any(start<=a<start+size for start,size in definition['excludedCode']):patch_data[a]=value
from utils.doorsmanager import colors2plm
cases=[]
for color in range(1,9):
 values=[color if d['canRandom'] else 0 for d in locations];table=(C.c_uint8*58)(*values)
 assert l.sm_doors_configure(0,table,58,catalog['sha256'].encode());activate(0)
 expected=bytearray(original_runtime)
 for address,value in patch_data.items():expected[address]=value
 for d,value in zip(locations,values):
  if value:
   a=d['address'];expected[a:a+2]=colors2plm[colors[value]][d['facing']].to_bytes(2,'little')
   if value==4:expected[a+5]=0x90
 for p in m['placements']:expected[p['address']:p['address']+2]=p['plm'].to_bytes(2,'little')
 assert select(items,100,m['sha256'].encode())
 actual=C.string_at(romptr,0x300000)
 assert actual==expected,(color,[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:10])
 wrong=(Item*100)(*items);wrong[0].address=0;assert not select(wrong,100,m['sha256'].encode()) and C.string_at(romptr,0x300000)==actual
 assert not l.sm_doors_configure(0,table,57,catalog['sha256'].encode())
 table[0]=99;assert not l.sm_doors_configure(0,table,58,catalog['sha256'].encode())
 C.memset(ram+0xd8b0,0,64);initial()
 for d,value in zip(locations,values):assert bool(read(0xd8b0+d['openedBit']//8,1)[0]&(1<<(d['openedBit']%8)))==(value==0)
 assert select(None,0,None) and C.string_at(romptr,0x300000)==original_runtime
 cases.append(dict(color=colors[color],sha256=hashlib.sha256(actual).hexdigest()))
# Native handlers execute the exact admitted hit policy, including bomb
# rejection and owning both Spazer/Plasma while either beam is equipped.
shots=[0,1,2,4,8,12,0x100,0x200,0x300,0x500,0x501,0x50c,0x1001,0x100c,0x8001]
hits=[]
for address,mask in [(0x84f2db,1),(0x84f2ea,2),(0x84f2f9,4),(0x84f31b,8),(0x848560,0)]:
 for shot,collected in itertools.product(shots,[0,4,8,12]):
  C.memmove(ram,baseline,len(baseline));put(0x9a8,collected);put(0x1d77,shot);put(0x1d27,0x1234);put(0xdebc,0x5678);put(0xde1c,19)
  check(address,0)
  allowed=bool((shot&0xf00)!=0x500 and (shot&(12 if mask in (4,8) and collected==12 else mask))) if mask else bool(shot and (shot&0xf00)==0x100)
  assert word(0x1d77)==0 and word(0x1d27)==(0x5678 if allowed else 0x1234) and word(0xde1c)==(1 if allowed else 19),(hex(address),hex(shot),collected,allowed)
  hits.append(dict(address=address,shot=shot,collected=collected,opens=allowed))
assert l.sm_cpu_opcodes()==0;l.sm_shutdown();assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(out/'verification.json').write_text(json.dumps(dict(colors=cases,hits=hits,cpu=0,sourceRomUnchanged=True,nativeSha256=hashlib.sha256(libpath.read_bytes()).hexdigest(),scope=__doc__),indent=2))
print('NATIVE_DOOR_COLORS_PASS',len(cases),len(hits),flush=True)
