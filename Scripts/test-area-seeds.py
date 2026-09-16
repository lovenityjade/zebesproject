#!/usr/bin/env python3
"""Actual area/light/boss/door/start generations and fresh solver progressions."""
import ctypes as C
import hashlib,json,os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
lib=C.CDLL(str(root/'areas/libsm_native.so'))
lib.sm_randomizer_generate.argtypes=[C.c_char_p,C.c_char_p,C.c_char_p,C.c_int]
out=root/'area-seeds';out.mkdir(exist_ok=True)
cases=[
    dict(areaRandomization='full'),
    dict(areaRandomization='light'),
    dict(areaRandomization='full',bossRandomization='on',doorsColorsRando='on',allowGreyDoors='on'),
    dict(areaRandomization='full',startLocation='Golden Four'),
    dict(areaRandomization='light',startLocation='Red Brinstar Elevator'),
    dict(areaRandomization='off'),
]
reports=[]
for i,options in enumerate(cases):
    request=dict(seed=15092600+i,skill='regular',noAdvancedTechs=i in (1,3),options=dict(layoutPatches='on',variaTweaks='on',**options))
    b=C.create_string_buffer(2097152)
    n=lib.sm_randomizer_generate(str(root/'Randomizer').encode(),json.dumps(request).encode(),b,len(b));assert 0<n<=len(b)
    result=json.loads(b.value)
    (out/f'candidate-{i:02d}.json').write_text(json.dumps(result,indent=2)+'\n')
    assert result.get('ok'),result
    m=result['manifest'];v=m['solverVerification'];context=m['nativeContext'];topology=context['topology']
    assert v['allItemsReachable'] and v['completionVerified'] and v['reachableItemCount']==100
    assert v['areaConnections']==topology['areaPairs']
    assert ('native-area-connections-v1' in m['requiredNativeBehavior'])==(i!=5)
    assert len(topology.get('nativeAreas',{}).get('destinations',[]))==(32 if i!=5 else 0)
    assert len(context['world']['initialDoors']['opened'])>=9
    assert context['world']['initialDoors']['opened']==m['tracker']['settings']['initialDoors']
    assert 'native-initial-doors-v1' in m['requiredNativeBehavior']
    if i!=5:assert m['tracker']['topology']['nativeAreas']==topology['nativeAreas']
    if i==2:
        assert topology['mode']=='area-boss' and 'doorColors' in context
        assert context['doorColors']['doors']==m['tracker']['topology']['doors']
    if i in (3,4):assert context['start']==options['startLocation']
    (out/f'seed-{i:02d}.json').write_text(json.dumps(result,indent=2)+'\n')
    reports.append(dict(seed=m['seed'],fingerprint=m['sha256'],mode=topology['mode'],start=context['start'],spawn=context['spawn'],checks=v['reachableItemCount'],noAdvanced=m['rules']['noAdvancedTechs']))
    print('AREA_SEED_PASS',i,m['seed'],topology['mode'],context['start'],flush=True)
(out/'verification.json').write_text(json.dumps(dict(seeds=reports,nativeSha256=hashlib.sha256((root/'areas/libsm_native.so').read_bytes()).hexdigest()),indent=2)+'\n')
