#!/usr/bin/env python3
"""Suit damage semantics from original VARIA patch sites, gaming-pc only."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
fixture=(root/'gameplay/test-native-gameplay-patches.py').read_text().split('assert l.sm_seed_rules_capabilities()')[0]
fixture=fixture.replace('gameplay/libsm_native.so','suits/libsm_native.so').replace("root/'gameplay/native'","root/'suits/native'")
exec(compile(fixture,'suits-fixture','exec'))
assert l.sm_seed_rules_capabilities()==2047
heat=routine('PalPreInstr_SamusInHeat',None,C.c_uint16)
periodic=routine('Samus_HandlePeriodicDamage',None)
contact=routine('SuitDamageDivision',C.c_uint16,C.c_uint16)
metroid=routine('Metroid_Func_5',None,C.c_uint16)
rows=[]
# Index: neither suit, Varia alone, Gravity alone, both. Balanced's Varia
# quarters periodic damage: the upstream patch changes BIT #$20 to BIT #$01.
spec={0:([1,2,4,4],[1,2,4,4],[1,0,0,0]),512:([1,2,4,4],[1,4,1,4],[1,0,1,0]),1024:([1,2,2,4],[1,2,2,4],[1,0,1,0])}
for randomized in [True,False]:
 assert select(items,100,m['sha256'].encode()) if randomized else select(None,0,None)
 for flags in spec:
  enemydiv,envdiv,heat_enabled=spec[flags if randomized else 0]
  for i,suits in enumerate([0,1,32,33]):
   reset(flags);setv('equipped_items',suits)
   assert contact(101)==101//enemydiv[i]
   setv('samus_periodic_damage',0);setv('samus_periodic_subdamage',0);setv('samus_health',100)
   heat(0);assert getv('samus_periodic_subdamage')==0x4000*heat_enabled[i]
   for incoming in [0x4000,0x18000,0x34567,0x800ff]:
    reset(flags);setv('equipped_items',suits);setv('time_is_frozen_flag',0)
    setv('samus_health',100);setv('samus_subunit_health',0)
    setv('samus_periodic_damage',incoming>>16);setv('samus_periodic_subdamage',incoming&65535)
    expected=incoming if envdiv[i]==1 else (incoming//envdiv[i])&0xffff00
    periodic();actual=(getv('samus_health')<<16)|getv('samus_subunit_health')
    assert actual==(100<<16)-expected,(randomized,flags,suits,incoming,actual,expected)
    assert getv('samus_periodic_damage')==getv('samus_periodic_subdamage')==0
   put(0x7804,0xffff);metroid(0)
   assert word(0x7804)==0xffff-0xc000//enemydiv[i]
   rows.append(dict(randomized=randomized,flags=flags,suits=suits,enemyDivisor=enemydiv[i],periodicDivisor=envdiv[i],heat=heat_enabled[i]))
assert l.sm_cpu_opcodes()==0;l.sm_shutdown()
(root/'suits/native-results.json').write_text(json.dumps(dict(cases=rows,periodicCases=96,nativeSha256=hashlib.sha256(libpath.read_bytes()).hexdigest(),cpu=0),indent=2)+'\n')
print('NATIVE_SUITS_PASS',len(rows),flush=True)
