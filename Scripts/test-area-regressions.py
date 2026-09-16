#!/usr/bin/env python3
"""Run existing boss/world regressions against the isolated area candidate."""
import os
import subprocess
import sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
for name in ['test-native-boss-connections.py','test-world-data.py']:
    source=(root/name).read_text()
    if name=='test-native-boss-connections.py':
        source=source.replace('connections/libsm_native.so','areas/libsm_native.so').replace("root/'connections/native'","root/'areas/boss-regression'")
    else:
        source=source.replace("libpath=root/'world-data/libsm_native.so'","libpath=root/'areas/libsm_native.so'")
        source=source.replace("out=root/'world-data'","out=root/'areas/world-regression'")
    log=root/'areas'/(name.removesuffix('.py')+'.log')
    with log.open('w') as stream:
        subprocess.run([sys.executable,'-c',source,str(root)],stdout=stream,stderr=subprocess.STDOUT,check=True)
    print('AREA_REGRESSION_PASS',name,flush=True)
