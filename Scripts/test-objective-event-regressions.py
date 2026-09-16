#!/usr/bin/env python3
"""Existing map, seed-transition, tracker, save and Vulkan checks on event core."""
import os,subprocess,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
for name in ['test-native-map-exploration.py','test-map-exploration-regressions.py','test-map-exploration-transitions.py','test-map-exploration-unreal.py']:
    source=(root/name).read_text().replace('map-exploration','objective-events')
    if name=='test-native-map-exploration.py':source=source.replace("root/'objective-events/native-results.json'","root/'objective-events/map-results.json'")
    target=root/'objective-events'/(name+'.log')
    with target.open('w') as log:
        result=subprocess.run([sys.executable,'-c',source,str(root)],stdout=log,stderr=subprocess.STDOUT)
    if result.returncode:raise SystemExit(f'{name} failed ({result.returncode}); see {target}')
    print('OBJECTIVE_EVENTS_REGRESSION_PASS',name,flush=True)
