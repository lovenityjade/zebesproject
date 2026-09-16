#!/usr/bin/env python3
"""Targeted actual native Mirror enemy/room routines against source bytes."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
fixture=(root/'test-native-tweaks.py').read_text().split('for slot,names in enumerate')[0]
fixture=fixture.replace('world-data/libsm_native.so','mirror/libsm_native.so').replace("root/'tweaks/native'","root/'mirror/native'")
exec(compile(fixture,'mirror-ai-fixture','exec'))
l.sm_mirror_configure.argtypes=[C.c_int,C.c_int,C.c_char_p]
digest=json.loads((root/'mirror/scrolls.json').read_text())['sha256'].encode()
reference=(root/'mirror/mirror-source-data.bin').read_bytes();pristine=rom.read_bytes()
romptr=C.c_void_p.from_address(base+symbols['g_rom']).value
pc=lambda a:((a>>16)&127)*32768+(a&32767)
for slot in [0,1]:
 assert l.sm_start_configure(slot,0,None,0,catalog['sha256'].encode())
 assert l.sm_mirror_configure(slot,int(slot==0),digest)
def clean():
 C.memmove(ram,baseline,len(baseline));C.memset(ram+0xf78,0,64);C.memset(ram+0x7800,0,64)
 put(0xe54,0);put(0x7a5,64);put(0xf7a,512);put(0xf7e,512)
 put(0xf82,8);put(0xf84,8);put(0x1840,0)
results=[]
for slot in [1,0]:
 assert activate(slot,1) and select(items,100,m['sha256'].encode())
 source_bytes=reference if slot==0 else pristine
 # Source-authored animation/velocity data, including C literal replacements.
 for a,n in [(0xa7e824,20),(0xa7e87c,20),(0xa7e8f6,8),(0xa7e908,2),(0xa7e90c,2),(0x84b876,20)]:
  assert C.string_at(romptr+pc(a),n)==source_bytes[pc(a):pc(a)+n],hex(a)
 for direction in [0,1,2]:
  clean();put(0xfb4,direction<<8);put(0xfb6,0x0160);put(0xf92,128)
  routine('Boulder_Init',None)()
  right=direction==0 or slot==0 and direction==2
  assert word(0xfb0)==direction and word(0x7800)==(96 if right else 0xffa0)
  for dx in [-32,32]:
   put(0xfa8,0x879a);put(0xaf6,word(0xf7a)+dx);put(0xafa,word(0xf7e)+16)
   routine('Boulder_Func_1',None)()
   assert (word(0xfa8)!=0x879a)==((dx>0)==right),(slot,direction,dx)
   results.append(dict(kind='boulder',slot=slot,direction=direction,dx=dx))
 # The patched CMP immediate and actual branch opcode determine each boundary.
 for number,address in [(16,0xa7ec9d),(17,0xa7ecc1),(18,0xa7ece5),(19,0xa7ed12),(20,0xa7ed30),(25,0xa7eea0)]:
  a=pc(address);assert source_bytes[a]==0xc9
  threshold=int.from_bytes(source_bytes[a+1:a+3],'little');branch=source_bytes[a+3];assert branch in [0x10,0x30]
  for x in [0,threshold-1,threshold,threshold+1,65535]:
   clean();put(0xf7a,x);put(0xfb2,0x7777)
   routine('Etecoon_Func_'+str(number),None,C.c_uint16)(0)
   negative=bool(((x-threshold)&65535)&32768)
   should_change=negative if branch==0x10 else not negative
   assert (word(0xfb2)!=0x7777)==should_change,(slot,number,x,threshold,branch,word(0xfb2))
   results.append(dict(kind='etecoon-boundary',slot=slot,function=number,x=x))
 for direction in [0,1]:
  clean();put(0xfb0,1);put(0xfb4,direction)
  routine('Etecoon_Func_11',None,C.c_uint16)(0)
  address=0xa7ebdc if direction else 0xa7ebf0
  expected=int.from_bytes(source_bytes[pc(address)+1:pc(address)+3],'little')
  speedaddress=0xa7e908 if direction else 0xa7e90c
  assert word(0xf92)==expected and word(0xfac)==int.from_bytes(source_bytes[pc(speedaddress):pc(speedaddress)+2],'little')
  results.append(dict(kind='etecoon-rebound',slot=slot,direction=direction))
 clean();put(0x9a4,0x2000);put(0x197a,256)
 routine('PlmPreInstr_WakeAndLavaIfBoosterCollected',None,C.c_uint16)(0)
 assert word(0x197c)==int.from_bytes(source_bytes[pc(0x84b810):pc(0x84b810)+2],'little')
 for x in [0,1,2784,2785]:
  clean();put(0xaf6,x);put(0x1980,0)
  routine('PlmPreInstr_WakePLMAndStartFxMotionSamusFarLeft',None,C.c_uint16)(0)
  boundary=int.from_bytes(source_bytes[pc(0x84b82b):pc(0x84b82b)+2],'little')
  assert bool(word(0x1980))==(x<=boundary)
 clean();put(0x1d77,0);put(0xaf6,0);put(0x1978,0xffff)
 routine('PlmPreInstr_AdvanceLavaSamusMovesLeft',None,C.c_uint16)(0)
 assert word(0x1978)==int.from_bytes(source_bytes[pc(0x84b878):pc(0x84b878)+2],'little')
 assert word(0x197c)==int.from_bytes(source_bytes[pc(0x84b87a):pc(0x84b87a)+2],'little')
 clean();put(0x1c27,0);put(0x1c37,0xd113)
 tile=8*64+source_bytes[pc(0x84d16b)];put(0x10002+2*tile,255)
 routine('PlmPreInstr_DeletePlmAndSpawnTriggerIfBlockDestroyed',None,C.c_uint16)(0)
 assert word(0x1c37)==0 and word(0x10002+2*tile)&0xf000==0xb000
 for function,source_x,header,area in [
  ('PlmSetup_D6DA_LowerNorfairChozoHandTrigger',0x84d1da,0xd113,2),
  ('PlmSetup_D6F2_WreckedShipChozoHandTrigger',0x84d673,0xd6f8,3)]:
  clean();C.memset(ram+0x1c37,0,80);C.memset(ram+0xcd20,0xa5,50)
  put(0x79f,area);C.memset(ram+0xd828,255,8);put(0x9a4,0x200)
  put(0xb02,3);put(0xa1c,0x1d);put(0x1c87,0)
  routine(function,C.c_uint8,C.c_uint16)(0)
  assert word(0x1c37+78)==header
  assert word(0x1c87+78)==2*(29*64+source_bytes[pc(source_x)])
  if area==3:
   index=9 if slot==0 else 7
   assert read(0xcd20+index,2)==b'\x02\x02' and read(0xcd20+index+6,2)==b'\x01\x01'
  results.append(dict(kind='chozo-trigger',slot=slot,function=function))
 results.append(dict(kind='lava-and-destroyed-block',slot=slot))
assert l.sm_cpu_opcodes()==0;l.sm_shutdown()
(root/'mirror/ai-results.json').write_text(json.dumps(dict(cases=results,cpu=0,nativeSha256=hashlib.sha256(libpath.read_bytes()).hexdigest(),scope='Native Boulder/Etecoon branch boundaries, rebound, authored data, lava and destroyed-block triggers; public Mirror remains incomplete'),indent=2)+'\n')
print('MIRROR_AI_PASS',len(results),flush=True)
