#!/usr/bin/env python3
"""Reuse independent native sprite, tracker and mixed-bank regressions."""
import os,subprocess,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
for name in ['test-native-map-icons.py','test-map-icon-regressions.py']:
    source=(root/name).read_text().replace('map-icons','map-exploration')
    if name=='test-native-map-icons.py':source=source.replace("root/'map-exploration/audit.json'","root/'map-icons/audit.json'")
    with (root/'map-exploration'/(name+'.log')).open('w') as log:
        subprocess.run([sys.executable,'-c',source,str(root)],stdout=log,stderr=subprocess.STDOUT,check=True)
    print('MAP_EXPLORATION_REGRESSION_PASS',name,flush=True)
