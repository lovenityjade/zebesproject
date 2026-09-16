#!/usr/bin/env python3
"""Objective event and map regressions against the configured-goal candidate."""
import os,subprocess,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
for name in ['test-native-objectives.py','test-native-objective-events.py','test-native-map-exploration.py','test-map-exploration-regressions.py']:
    source=(root/name).read_text().replace('objective-events','objectives').replace('map-exploration','objectives')
    if name=='test-native-map-exploration.py':source=source.replace("root/'objectives/native-results.json'","root/'objectives/map-results.json'")
    target=root/'objectives'/(name+'.log')
    with target.open('w') as log:
        result=subprocess.run([sys.executable,'-c',source,str(root)],stdout=log,stderr=subprocess.STDOUT)
    if result.returncode:raise SystemExit(f'{name} failed ({result.returncode}); see {target}')
    print('OBJECTIVES_REGRESSION_PASS',name,flush=True)
