#!/usr/bin/env python3
"""Run the full condition suite against the companion-snapshot candidate."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-objectives.py').read_text()
source=source.replace('objectives/libsm_native.so','objectives-logic/libsm_native.so')
source=source.replace("root/'objectives/native'","root/'objectives-logic/conditions'")
source=source.replace("root/'objectives/conditions-results.json'","root/'objectives-logic/conditions-results.json'")
exec(compile(source,'objective-logic-native-fixture','exec'))
