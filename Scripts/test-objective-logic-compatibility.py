#!/usr/bin/env python3
"""Embedded standard/relic generation and historical tracker ABI regression."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-objective-seeds.py').read_text()
source=source.replace('objectives-binding/libsm_native.so','objectives-logic/libsm_native.so')
source=source.replace("out=root/'objective-seeds'","out=root/'objectives-logic/compatibility-seeds'")
exec(compile(source,'objective-compatibility-generation','exec'))
source=(root/'test-live-tracker-native.py').read_text()
source=source.replace("root/'Native/build/libsm_native.so'","root/'objectives-logic/libsm_native.so'")
source=source.replace("out=root/'tracker-results'","out=root/'objectives-logic/historical-tracker'")
exec(compile(source,'objective-historical-tracker','exec'))
