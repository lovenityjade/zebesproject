#!/usr/bin/env python3
"""Ten source Animals Surprise variants, native callbacks and slot transactions."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
fixture=(root/'test-native-tweaks.py').read_text().split('for slot,names in enumerate')[0]
fixture=fixture.replace('world-data/libsm_native.so','animals/libsm_native.so').replace("root/'tweaks/native'","root/'animals/native'")
exec(compile(fixture,'animals-fixture','exec'))
animals=json.loads((root/'animals/native_animals.json').read_text());digest=animals['sha256'].encode()
l.sm_animals_configure.argtypes=[C.c_int,C.c_int,C.c_char_p]
configure=l.sm_animals_configure
room=routine('CallRoomSetupCode',None,C.c_uint32);door=routine('CallDoorDefSetupCode',None,C.c_uint32)
romptr=C.c_void_p.from_address(base+symbols['g_rom']).value;rombytes=lambda:C.string_at(romptr,0x300000)
for slot in range(3):assert l.sm_start_configure(slot,0,None,0,catalog['sha256'].encode())
assert not configure(-1,1,digest) and not configure(4,1,digest) and not configure(0,11,digest) and not configure(0,1,b'bad')
assert activate(1,1) and select(None,0,None);original=rombytes()
assert select(items,100,m['sha256'].encode());normal=rombytes()
results=[]
for variant in animals['variants']:
 mode=variant['id'];assert configure(0,mode,digest)
 assert not l.sm_animals_state()
 assert activate(0,1) and select(items,100,m['sha256'].encode())
 assert l.sm_animals_state()==mode
 expected=bytearray(normal)
 for span in variant['data']:expected[span['address']:span['address']+len(span['data'])]=bytes(span['data'])
 assert rombytes()==expected,(mode,'data')
 before=rombytes();assert activate(1,1);bad=(Item*100)(*items);bad[0].address=0
 assert not select(bad,100,m['sha256'].encode()) and rombytes()==before and l.sm_animals_state()==mode
 if mode in [3,4,8,9,10]:
  for escape in [False,True]:
   C.memmove(ram,baseline,len(baseline));put(0xd820,0x4000 if escape else 0)
   put(0xd829,0x1234);put(0xd82b,0x5678);room(0x8ff006)
   assert word(0x7b5)==0xf000
   assert word(0xd829)==(0x1234 if mode==4 else 0) and word(0xd82b)==(0x5678 if mode==4 else 0)
   if mode!=4:
    put(0x7b5,0x5555);C.memset(ram+0xd8bb,0xa5,8);put(0x1cd5,0x5555)
    room(0x8ff01c)
    assert word(0x7b5)==({3:0xf06f,8:0xf038,9:0xf03b,10:0xf05e}[mode] if escape else 0x5555)
    if escape and mode in [8,9,10]:assert word({8:0xd8bc,9:0xd8c0,10:0xd8bb}[mode])==0
    if escape and mode in [3,10]:
     assert word(0x1cd5)==(0xffff if mode==3 else 0xaaaa)
     offsets=[0x101be,0x101fe,0x1023e,0x1027e] if mode==3 else [0x100de,0x100fe,0x1011e,0x1013e]
     assert [word(a) for a in offsets]==[0x80ae,0x80ce,0x88ce,0x88ae]
     if mode==3:assert all(read(a,1)==b'\0' for a in [0x166c2,0x166e2,0x16702,0x16722])
 elif mode in [5,7]:
  for escape in [False,True]:
   C.memmove(ram,baseline,len(baseline));put(0xd820,0x4000 if escape else 0)
   put(0x946,0x0350);put(0x998,8);door(0x8fff20)
   assert word(0x946)==(0x20 if escape and mode==7 else 0x0350)
   assert word(0x998)==(38 if escape and mode==5 else 8)
 # Every surprise moves the Alcatraz bomb-block event from animals to escape.
 for flags in [0,0x4000,0x8000,0xc000]:
  C.memmove(ram,baseline,len(baseline));put(0xd820,flags);put(0x1c37,0xbb30)
  routine('PlmSetup_BB30_CrateriaMainstreetEscape',C.c_uint8,C.c_uint16)(0)
  assert bool(word(0x1c37))==bool(flags&0x4000)
 C.memmove(ram,baseline,len(baseline));put(0xd820,0);put(0xe54,0);C.memset(ram+0xf78,0,64)
 routine('EscapeEtecoon_Init',None)()
 assert word(0xf86)==(0xa000 if mode in [1,2] else 0xa400)
 assert select(items,100,m['sha256'].encode()) and not l.sm_animals_state() and rombytes()==normal
 results.append(dict(mode=mode,name=variant['name'],data=True,callbacks=True,rollback=True))
assert select(None,0,None) and rombytes()==original
C.memmove(ram,baseline,len(baseline));put(0xd820,0x4000);put(0x1c37,0xbb30)
routine('PlmSetup_BB30_CrateriaMainstreetEscape',C.c_uint8,C.c_uint16)(0);assert word(0x1c37)==0
assert l.sm_cpu_opcodes()==0;l.sm_shutdown()
(root/'animals/native-results.json').write_text(json.dumps(dict(variants=results,vanillaFullRomRestored=True,cpu=0,nativeSha256=hashlib.sha256(libpath.read_bytes()).hexdigest(),scope='Controlled data and native callbacks for all variants; source generation and full room transitions still required'),indent=2)+'\n')
print('ANIMALS_NATIVE_PASS',len(results),flush=True)
