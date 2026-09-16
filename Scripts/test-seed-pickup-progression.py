#!/usr/bin/env python3
"""Replay four verified solver logs with real native PLM grants and live logic.

Inject the location's collection-script entry in a stable room. Native PLM
code owns collection bits, quantities, equipment, messages and inventory.
Boss flags follow each solver step; traversal/combat are not simulated here.
"""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
source=(root/'Scripts/test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
source=source.replace("l=C.CDLL(str(root/'Native/build/libsm_native.so'))","l=C.CDLL(str(root/'pickup-incident/libsm_native-fixed.so'))")
exec(compile(source,'tracker-fixture','exec'))
out=root/'pickup-incident/SMTests-progression';out.mkdir(exist_ok=True)
sys.path.insert(0,str(root/'Randomizer/upstream'))
from rando.Items import ItemManager
rom_bytes=rom.read_bytes()
names=['ETank','Missile','Super','PowerBomb','Bomb','Charge','Ice','HiJump','SpeedBooster','Wave','Spazer','SpringBall','Varia','Gravity','XRayScope','Plasma','Grapple','SpaceJump','ScrewAttack','Morph','Reserve']
messages=[1,2,3,4,19,14,15,11,13,16,17,8,7,26,6,18,5,12,10,9,25]
bosses=[('Kraid',1,1),('Phantoon',3,1),('Draygon',4,1),('Ridley',2,1),('MotherBrain',5,2),('SporeSpawn',1,2),('Crocomire',2,2),('Botwoon',4,2),('GoldenTorizo',2,4)]
by_name={v['name']:i for i,v in enumerate(geometry)}
reports=[]
for path in [root/f'results/seed-{seed}.json' for seed in [14092026,14092027,14092028]]+[root/'pickup-incident/seed.json']:
 m=json.loads(path.read_text())['manifest'];t=m['tracker'];placements={p['location']:p for p in m['placements']}
 boot(f'seed-{m["seed"]}',True);l.sm_tracker_configure(1,1,1)
 assert l.sm_test_rendering(0) # native turbo render-skip; this replay checks logic, not pixels
 cases=[];visited=set()
 for log in m['solverVerification']['progressionLog']:
  name=log['location']
  if name not in placements:continue
  for boss,area,bit in bosses:
   if log['inventoryBefore'].get(boss,0):
    off=0xd828+area;C.c_uint8.from_address(ram+off).value|=bit
  s=snap();states=query(s);loc=by_name[name]
  assert states[loc] in [1,4],(m['seed'],name,states[loc],hex(s.acquired_items))
  for old in visited:assert states[by_name[old]]==0,(m['seed'],old)
  p=placements[name];header=p['plm'];item_index=((header-0xeed7)//4)%21
  assert names[item_index]==p['item']
  start=int.from_bytes(rom_bytes[0x20000+(header&32767)+2:0x20000+(header&32767)+4],'little')
  data=rom_bytes[0x20000+(start&32767):0x20000+(start&32767)+150]
  grant=start+data.index(b'\x99\x88')
  bit=int.from_bytes(rom_bytes[p['address']+4:p['address']+6],'little')&255
  before=[l.sm_seed_inventory(i) for i in range(7)]
  put(0x1c37,header);put(0x1cd7,0x8469);put(0x1d27,grant);put(0xde1c,1)
  put(0x1dc7,bit);put(0x1c87,0);put(0xdf0c,item_index*2)
  for _ in range(120):
   step()
   if l.sm_message_active():break
  else:raise AssertionError(('No message',m['seed'],name))
  expected=before.copy();item=p['item']
  if item in ['ETank','Missile','Super','PowerBomb','Reserve']:
   index,amount={'ETank':(0,100),'Missile':(1,5),'Super':(2,5),'PowerBomb':(3,5),'Reserve':(6,100)}[item];expected[index]+=amount
  else:
   item_data=ItemManager.Items[item];expected[4]|=item_data.ItemBits;expected[5]|=item_data.BeamBits
  actual=[l.sm_seed_inventory(i) for i in range(7)]
  assert actual==expected,(m['seed'],name,item,actual,expected)
  assert word(0x1c1f)==messages[item_index],(m['seed'],name,'message')
  assert l.sm_seed_location_collected(loc),(m['seed'],name,'collection')
  after=snap();assert after.acquired_items==expected[4] and after.acquired_beams==expected[5]
  cases.append(dict(location=name,item=item,visibility=p['visibility'],plm=hex(header),stateBefore=int(states[loc]),inventory=actual,message=word(0x1c1f)))
  visited.add(name)
  # Let the real message coroutine finish, then retire only this fixture PLM.
  for i in range(700):
   step(256 if i%60<2 else 0)
   if i>60 and not l.sm_message_active():break
  assert not l.sm_message_active();put(0x1c37,0)
  # Suit acquisition scripts may animate after their message. Settle native
  # animation before injecting the next item, keeping actual inventory intact.
  wait(300)
  if len(cases)%10==0:print('PROGRESSION',m['seed'],len(cases),flush=True)
 assert len(cases)==100 and sum(snap().collected)==100
 assert not l.sm_cpu_opcodes()
 reports.append(dict(seed=m['seed'],fingerprint=m['sha256'],skill=m['skill'],checks=cases))
 l.sm_shutdown();print('SEED_PICKUPS_PASS',m['seed'],100,flush=True)
report=dict(passed=True,seeds=reports,pickups=sum(len(r['checks']) for r in reports),cpuOpcodes=0,
 rendering='Native turbo render-skip after real boot; graphics verified separately by 63 rendered variant tests and ceiling collision capture',
 sourceRomUnchanged=hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash,scope=__doc__)
(out/'verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('SM_SEED_PICKUP_PROGRESSION_PASS',report['pickups'],flush=True)
