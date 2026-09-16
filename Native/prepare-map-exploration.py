#!/usr/bin/env python3
"""Both original minimap/boss exploration paths, guarded against source drift."""
from pathlib import Path
import sys
p=Path(sys.argv[1]);s=p.read_text()
for index in ['(uint16)(r20 + 4 * (R34 + R22))','v2']:
    old=f'  map_tiles_explored[{index}] |= kShr0x80[(room_x_coordinate_on_map + v0) & 7];'
    assert s.count(old)==1,old
    s=s.replace(old,f'  sm_map_exploration_mark({index},kShr0x80[(room_x_coordinate_on_map + v0) & 7]);')
p.write_text('#include "sm_map_exploration.h"\n'+s)
