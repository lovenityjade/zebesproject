#!/usr/bin/env python3
"""All 63 original item PLM grant/message paths, on an isolated gaming-pc core.

The fixture enters each ROM-authored collection script at SetItemCollected;
this covers native PLM dispatch, collection bits, grant, message and tracker
snapshot, but does not claim to exercise every room's collision/reveal path.
The companion ceiling reproduction covers actual shooting/jumping/collision.
"""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
source=(root/'Scripts/test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
source=source.replace("l=C.CDLL(str(root/'Native/build/libsm_native.so'))","l=C.CDLL(str(root/os.environ.get('PICKUP_LIB','Native/build/libsm_native.so')))")
exec(compile(source,'tracker-fixture','exec'))
out=root/'pickup-incident/SMTests-grants';out.mkdir(exist_ok=True)
sys.path.insert(0,str(root/'Randomizer/upstream'))
from rando.Items import ItemManager
rom_bytes=rom.read_bytes()
def rom84(a):return 0x20000+(a&32767)
def rw(a):return int.from_bytes(rom_bytes[rom84(a):rom84(a)+2],'little')
# Independent expected inventory and messages, ordered by original item PLMs.
names=['ETank','Missile','Super','PowerBomb','Bomb','Charge','Ice','HiJump','SpeedBooster','Wave','Spazer','SpringBall','Varia','Gravity','XRayScope','Plasma','Grapple','SpaceJump','ScrewAttack','Morph','Reserve']
messages=[1,2,3,4,19,14,15,11,13,16,17,8,7,26,6,18,5,12,10,9,25]
results=[]
for variant in range(3):
 for item_index,name in enumerate(names):
  boot(f'{variant}-{name}',True)
  header=0xeed7+84*variant+4*item_index
  start=rw(header+2);data=rom_bytes[rom84(start):rom84(start)+150]
  grant=start+data.index(b'\x99\x88')  # native SetItemCollected instruction
  # A stable PLM slot and real item ID; leave Samus and native pause intact.
  put(0x1c37,header);put(0x1cd7,0x8469);put(0x1d27,grant);put(0xde1c,1)
  put(0x1dc7,0x1a);put(0x1c87,0);put(0xdf0c,item_index*2)
  before=[l.sm_seed_inventory(i) for i in range(7)]
  for _ in range(120):
   step()
   if l.sm_message_active():break
  else:raise AssertionError(('No message',variant,name,hex(grant)))
  actual=[l.sm_seed_inventory(i) for i in range(7)];expected=before.copy()
  if name in ['ETank','Missile','Super','PowerBomb','Reserve']:
   index,amount={'ETank':(0,100),'Missile':(1,5),'Super':(2,5),'PowerBomb':(3,5),'Reserve':(6,100)}[name];expected[index]+=amount
  else:
   item=ItemManager.Items[name];expected[4]|=item.ItemBits;expected[5]|=item.BeamBits
  message=word(0x1c1f)
  assert actual==expected,(variant,name,actual,expected)
  assert message==messages[item_index],(variant,name,message)
  assert read(0xd870+3,1)[0]&4,('collection bit',variant,name)
  snapshot=snap();assert snapshot.acquired_items==expected[4] and snapshot.acquired_beams==expected[5]
  assert not l.sm_cpu_opcodes()
  results.append(dict(variant=['Visible','Chozo','Hidden'][variant],item=name,plm=hex(header),script=hex(grant),message=message,inventory=actual))
  l.sm_shutdown();print('PASS',variant,name,flush=True)
report=dict(passed=True,grants=results,cpuOpcodes=0,sourceRomUnchanged=hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash,
 scope=__doc__)
(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('SM_ITEM_GRANTS_PASS',len(results),flush=True)
