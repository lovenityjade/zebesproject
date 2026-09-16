#!/usr/bin/env python3
"""Objective conditions/oracle and historical native HUD/map compatibility."""
import hashlib,json,os,subprocess,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
out=root/'objectives-map';results=[];versions={}
for name in sys.argv[2:] or ['test-native-objectives.py','test-objective-live-state.py','test-native-map-icons.py','test-varia-ui.py','test-objective-pause-native.py','test-objective-hud-native.py']:
    source=(root/name).read_text()
    if name=='test-native-objectives.py':
        source=source.replace('objectives/libsm_native.so','objectives-map/libsm_native.so')
        source=source.replace("root/'objectives/native'","root/'objectives-map/conditions'")
        source=source.replace("root/'objectives/conditions-results.json'","root/'objectives-map/conditions-results.json'")
    elif name=='test-objective-live-state.py':source=source.replace('objectives-logic/','objectives-map/')
    elif name=='test-native-map-icons.py':
        source=source.replace('map-icons/libsm_native.so','objectives-map/libsm_native.so')
        source=source.replace("root/'map-icons/native'","root/'objectives-map/map-icons'")
        source=source.replace("root/'map-icons/results.json'","root/'objectives-map/map-icons-results.json'")
    else:
        source=source.replace("out=root/'varia-ui-results'","out=root/'objectives-map/legacy-ui'")
        source=source.replace("exec(compile(fixture,'tracker-fixture','exec'))",
            "fixture=fixture.replace('Native/build/libsm_native.so','objectives-map/libsm_native.so')\nexec(compile(fixture,'tracker-fixture','exec'))")
    target=out/(name+'.log')
    # Preserve the existing scripts' sibling-fixture lookup, while all runtime
    # mutations and reports remain in this isolated candidate directory.
    source=source.replace('objectives-pause','objectives-map')
    source=source.replace("root/'objectives-map/assets.json'","root/'objectives-pause/assets.json'")
    script='__file__='+repr(str(root/name))+'\n'+source
    versions[name]=hashlib.sha256(script.encode()).hexdigest()
    with target.open('w') as log:
        result=subprocess.run([sys.executable,'-c',script,str(root)],stdout=log,stderr=subprocess.STDOUT)
    if result.returncode:raise SystemExit(f'{name} failed ({result.returncode}); see {target}')
    results.append(name);print('OBJECTIVE_PRESENTATION_REGRESSION_PASS',name,flush=True)
(out/('regressions-rerun.json' if len(sys.argv)>2 else 'regressions.json')).write_text(json.dumps(dict(passed=results,executedScriptSha256=versions,
    nativeSha256=hashlib.sha256((out/'libsm_native.so').read_bytes()).hexdigest()),indent=2)+'\n')
