#!/usr/bin/env python3
"""Native Mirror data transactions versus independently patched source data."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
fixture=(root/'test-native-tweaks.py').read_text().split('for slot,names in enumerate')[0]
fixture=fixture.replace('world-data/libsm_native.so','mirror/libsm_native.so').replace("root/'tweaks/native'","root/'mirror/native'")
exec(compile(fixture,'mirror-data-fixture','exec'))
audit=json.loads((root/'mirror/data-audit.json').read_text())
locations=json.loads((root/'mirror/native_mirror_locations.json').read_text())
reference=(root/'mirror/mirror-source-data.bin').read_bytes()
assert hashlib.sha256(reference).hexdigest()==locations['patchedImageSha256']
romptr=C.c_void_p.from_address(base+symbols['g_rom']).value
rombytes=lambda:C.string_at(romptr,len(reference))
configure=l.sm_mirror_configure;configure.argtypes=[C.c_int,C.c_int,C.c_char_p]
digest=json.loads((root/'mirror/scrolls.json').read_text())['sha256'].encode()
active=routine('sm_mirror_active',C.c_int)
for slot in range(3):
 assert l.sm_start_configure(slot,0,None,0,catalog['sha256'].encode())
 assert configure(slot,int(slot!=1),digest)
assert activate(1,1) and select(None,0,None);vanilla=rombytes()
assert select(items,100,m['sha256'].encode());normal=rombytes()
placed={p.address:p.plm for p in items}
expected=bytearray(reference)
for loc in locations['locations']:
 a=loc['physicalAddress'];expected[a:a+2]=placed[loc['canonicalAddress']].to_bytes(2,'little')
# Independent fully-known-map fix, intentionally applied after Mirror data.
expected[0x1a820e:0x1a8210]=b'\x25\xcc'
checks=[]
for slot in [0,1,2,0,1,2]:
 assert activate(slot,1) and select(items,100,m['sha256'].encode())
 current=rombytes()
 if slot==1:assert current==normal,'normal slot retained Mirror data';continue
 assert active()
 for span in audit['spans']:
  a,n=span['address'],span['size']
  assert current[a:a+n]==expected[a:a+n],hex(a)
 for loc in locations['locations']:
  i=loc['index'];a=loc['physicalAddress']
  assert l.sm_seed_rom_item(i)==placed[loc['canonicalAddress']],loc['name']
  bit=int.from_bytes(current[a+4:a+6],'little')&255
  assert bit==loc['collectionBit'],loc['name']
 checks.append(slot)
# Pending/rejected plans must retain all bytes, not only enabled flags.
previous=rombytes();assert activate(1,1)
bad=(Item*100)(*items);bad[0].address=0
assert not select(bad,100,m['sha256'].encode()) and rombytes()==previous and active()
assert select(None,0,None) and rombytes()==vanilla and not active()
# Native callbacks: compare the PLM positions/directions authored by bank_8f.
cases=[('RoomSetup_AfterSavingAnimals',61,13,11,0xbb30,0xbb30),
 ('RoomSetup_AutoDestroyWallAfterEscape',16,30,135,0xb964,0xb964),
 ('RoomSetup_TurnWallIntoShotblocks',15,0,10,0xb9ed,0xb9ed),
 ('DoorCode_StartWreckedShipTreadmillWest',4,4,9,0xb64b,0xb64f),
 ('DoorCode_StartWreckedSkipTreadmill_East',4,4,9,0xb64f,0xb64b)]
callbacks=[]
for slot in [1,0]:
 assert activate(slot,1) and select(items,100,m['sha256'].encode())
 for name,x,mx,y,header,mheader in cases:
  C.memmove(ram,baseline,len(baseline));C.memset(ram+0x1c37,0,80);C.memset(ram+0x1ef5,0,12)
  put(0x7a5,64);C.memset(ram+0xd820,255,8)
  routine(name,None)()
  assert word(0x1c87+78)==2*(y*64+(mx if slot==0 else x)),(name,slot)
  assert word(0x1c37+78)==(mheader if slot==0 else header),(name,slot,hex(word(0x1c37+78)))
  callbacks.append(dict(name=name,mirror=slot==0))
assert select(None,0,None) and rombytes()==vanilla
assert l.sm_cpu_opcodes()==0;l.sm_shutdown()
result=dict(dataBytes=audit['dataBytes'],mirrorApplications=len(checks),locations=100,callbacks=callbacks,fullRomVanillaAndNormalRestoration=True,rejectedPlanPreserved=True,cpu=0,nativeSha256=hashlib.sha256(libpath.read_bytes()).hexdigest(),scope='Internal geometry/item transaction and five callbacks; unfinished AI and variant topology remain guarded')
(root/'mirror/data-results.json').write_text(json.dumps(result,indent=2)+'\n')
print('MIRROR_NATIVE_DATA_PASS',result['dataBytes'],len(callbacks),flush=True)
