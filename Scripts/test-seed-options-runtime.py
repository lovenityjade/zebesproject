#!/usr/bin/env python3
"""Round-trip real native item plans and query the in-process live oracle."""
import ctypes as C,hashlib,json,os,subprocess,sys,uuid
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
fixtures=root/(sys.argv[2] if len(sys.argv)>2 else 'menu-options')
assert fixtures.parent==root and fixtures.is_dir()
libpath=root/(sys.argv[3] if len(sys.argv)>3 else 'menu-options/libsm_native.so');l=C.CDLL(str(libpath))
class Item(C.Structure):_fields_=[('address',C.c_uint32),('plm',C.c_uint16),('kind',C.c_uint16)]
class Indicator(C.Structure):_fields_=[('location',C.c_uint16),('plm',C.c_uint16)]
l.sm_seed_stage.argtypes=[C.POINTER(Item),C.c_int,C.c_char_p];l.sm_init.argtypes=[C.c_char_p,C.c_char_p];l.sm_error.restype=C.c_char_p
l.sm_tracker_evaluate.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
rom=next((root/'roms').glob('*.sfc'));original=rom.read_bytes();romhash=hashlib.sha256(original).hexdigest()
symbols={x.split()[-1]:int(x.split()[0],16) for x in subprocess.check_output(['nm','-an',str(libpath)],text=True).splitlines() if len(x.split())==3 and x.split()[1].lower()=='t'}
base=C.cast(l.sm_init,C.c_void_p).value-symbols['sm_init']
select=C.CFUNCTYPE(C.c_int,C.POINTER(Item),C.c_int,C.c_char_p)(base+symbols['sm_seed_select_plan'])
results=[]
for i,fixture in enumerate(sorted(fixtures.glob('seed-*.json'))):
 m=json.loads(fixture.read_text())['manifest'];t=m['tracker'];items=(Item*100)(*[Item(p['address'],p['plm'],p.get('kind',0)) for p in m['placements']])
 assert l.sm_seed_stage(items,100,m['sha256'].encode())
 save=fixtures/m['sha256']/('native-'+uuid.uuid4().hex+'.sram');save.parent.mkdir(exist_ok=True);assert l.sm_init(str(rom).encode(),str(save).encode()),l.sm_error()
 if 'native-start-v1' in m.get('requiredNativeBehavior',[]):
  world=m['nativeContext']['world'];patches=(C.c_uint8*len(world['dataPatchIds']))(*world['dataPatchIds'])
  l.sm_start_configure.argtypes=[C.c_int,C.c_int,C.POINTER(C.c_uint8),C.c_int,C.c_char_p]
  entries=(Indicator*len(world.get('indicators',[])))(*[Indicator(e['locationId'],e['plm']) for e in world.get('indicators',[])])
  l.sm_start_configure_world.argtypes=l.sm_start_configure.argtypes+[C.POINTER(Indicator),C.c_int]
  assert l.sm_start_configure_world(3,world['startSpawn'],patches,len(patches),world['catalogSha256'].encode(),entries,len(entries))
  activate=C.CFUNCTYPE(C.c_int,C.c_int,C.c_int)(base+symbols['sm_start_activate']);assert activate(3,1)
  assert select(items,100,m['sha256'].encode())
 assert [l.sm_seed_rom_item(n) for n in range(100)]==[p['plm'] for p in m['placements']]
 assert select(None,0,None)
 assert [l.sm_seed_rom_item(n) for n in range(100)]==[int.from_bytes(original[p['address']:p['address']+2],'little') for p in m['placements']]
 assert select(items,100,m['sha256'].encode())
 assert [l.sm_seed_rom_item(n) for n in range(100)]==[p['plm'] for p in m['placements']]
 assert l.sm_cpu_opcodes()==0;l.sm_shutdown()
 routes=m['solverVerification']['progressionLog'];lookups={x['type']:x for x in t['items']};queries=[]
 for n in sorted(set([0,5,20,50,len(routes)-1])):
  inv=routes[n]['inventoryBefore'];native=dict(items=0,beams=0,health=99+100*inv.get('ETank',0),reserve=100*inv.get('Reserve',0),missiles=5*inv.get('Missile',0),supers=5*inv.get('Super',0),powerBombs=5*inv.get('PowerBomb',0),bosses=[0]*8,doors=[0]*64,collected=[0]*100)
  for name in inv:
   if name in lookups:native['items']|=lookups[name]['itemBits'];native['beams']|=lookups[name]['beamBits']
  for name,area,mask in [('Kraid',1,1),('Phantoon',3,1),('Draygon',4,1),('Ridley',2,1),('MotherBrain',5,2),('SporeSpawn',1,2),('Crocomire',2,2),('Botwoon',4,2),('GoldenTorizo',2,4)]:
   if inv.get(name):native['bosses'][area]|=mask
  q=dict(randomized=True,settings=t['settings'],topology=t['topology'],inventory=native)
  b=C.create_string_buffer(262144);size=l.sm_tracker_evaluate(str(root/'Randomizer').encode(),json.dumps(q).encode(),b,len(b));assert 0<size<=len(b);r=json.loads(b.value);assert r['ok'],r
  assert r['maximumDifficulty']==t['settings']['maximumDifficulty'] and len(r['checks'])==100
  if 'native-start-v1' in m.get('requiredNativeBehavior',[]):
   assert r['startAccessPoint']==t['settings']['start']
   assert r['returnAccessPoint']==('Landing Site' if t['settings']['start']=='Ceres' else t['settings']['start'])
  allowed={k for k,v in t['settings']['knows'].items() if v['enabled']}
  for c in r['checks']:
   if c['state']==1:assert c['safe'] and c['difficulty']<=r['maximumDifficulty'] and (set(c['techniques']) & set(t['settings']['knows']))<=allowed,(i,n,c)
  queries.append(dict(step=n,green=sum(c['state']==1 for c in r['checks'])))
 results.append(dict(seed=m['seed'],nativePlanReload=True,vanillaRestored=True,queries=queries))
 print('SEED_OPTIONS_RUNTIME',i,'PASS',flush=True)
assert hashlib.sha256(rom.read_bytes()).hexdigest()==romhash
(fixtures/'runtime-verification.json').write_text(json.dumps(results,indent=2));print('SEED_OPTIONS_RUNTIME_PASS',flush=True)
