#!/usr/bin/env python3
"""Existing tracker-pixel and slot-persistence suites on the map-icon candidate."""
import os,subprocess,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-live-tracker-native.py').read_text()
for old,new in [
    ("root/'Native/build/libsm_native.so'","root/'map-icons/libsm_native.so'"),
    ("out=root/'tracker-results'","out=root/'map-icons/tracker-regression'"),
    ("('plm',C.c_uint16)]","('plm',C.c_uint16),('kind',C.c_uint16)]"),
    ("Item(i['address'],i['plm'])","Item(i['address'],i['plm'],i.get('kind',0))")]:
    assert old in source,old
    source=source.replace(old,new)
with (root/'map-icons/tracker-regression.log').open('w') as stream:
    subprocess.run([sys.executable,'-c',source,str(root)],stdout=stream,stderr=subprocess.STDOUT,check=True)
print('MAP_ICON_TRACKER_REGRESSION_PASS',flush=True)
source=(root/'test-native-start-slots.py').read_text()
assert "out=root/('start-slot-results'" in source
source=source.replace("out=root/('start-slot-results'","out=root/('map-icons/slot-regression'")
with (root/'map-icons/slot-regression.log').open('w') as stream:
    subprocess.run([sys.executable,'-c',source,str(root),'00,02','area-seeds','map-icons/libsm_native.so'],stdout=stream,stderr=subprocess.STDOUT,check=True)
print('MAP_ICON_SLOT_REGRESSION_PASS',flush=True)
