#!/usr/bin/env python3
"""Collision-driven discovery of relocated portals in five generated seeds."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-generated-area-transitions.py').read_text()
source=source.replace("root/'areas/generated-transitions'","root/'map-exploration/generated-transitions'")
# Nested source loads must use the candidate without replacing old proofs.
source=source.replace("exec(compile(source,'generated-area-fixture','exec'))", "source=source.replace('areas/libsm_native.so','map-exploration/libsm_native.so')\nexec(compile(source,'generated-area-fixture','exec'))")
source=source.replace("results=[]\nfor seed", "audit=json.loads((root/'map-exploration/audit.json').read_text())\nresults=[]\nfor seed")
source=source.replace("for name in ['Crocomire Room Top','West Ocean Left','Crab Shaft Right']:", "for name in [p['name'] for p in audit['portals'] if p['relocated']]:")
old="        assert collide(None)==0 and l.sm_state()==9"
new="""        C.memset(ram+0x7f7,0,256);C.memset(ram+0xcd52,0,2048)
        routine('UpdateMinimap',None)()
        assert collide(None)==0 and l.sm_state()==9"""
assert old in source;source=source.replace(old,new)
old="        label=f'{seed:02d}-{i:02d}-to-{j:02d}';capture(label)"
new="""        for endpoint in {i,j}:
            p=audit['portals'][endpoint]
            if not p['relocated']:continue
            offset=(0x7f7 if p['area']==word(0x79f) else 0xcd52+256*p['area'])+p['byte']
            assert read(offset,1)[0]&p['mask'],(seed,name,aps[j]['name'],p)
        counts=[l.sm_map_exploration_value(r,0) for r in range(12)]
        routine('SaveToSram',None,C.c_uint16)(0)
        C.memset(ram+0x7f7,0,256);C.memset(ram+0xcd52,0,2048)
        assert routine('LoadFromSram',C.c_uint8,C.c_uint16)(0)==0
        routine('LoadMirrorOfExploredMapTiles',None)()
        assert [l.sm_map_exploration_value(r,0) for r in range(12)]==counts
        for endpoint in {i,j}:
            p=audit['portals'][endpoint]
            if p['relocated']:assert read(0xcd52+256*p['area']+p['byte'],1)[0]&p['mask']
        label=f'{seed:02d}-{i:02d}-to-{j:02d}';capture(label)"""
assert old in source;source=source.replace(old,new)
source=source.replace("states=states,transitionFrames=frame+1,room=l.sm_room(),capture=label+'.png'))", "states=states,transitionFrames=frame+1,room=l.sm_room(),capture=label+'.png',explorationCounts=counts,portalSaveReload=True))")
exec(compile(source,'map-exploration-transitions','exec'))
