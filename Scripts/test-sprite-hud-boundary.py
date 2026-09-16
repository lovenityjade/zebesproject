#!/usr/bin/env python3
"""Render a real Samus below a one-screen room; HUD must not inherit OAM wrap."""
import sys,uuid
from pathlib import Path
root=Path(sys.argv[1]).resolve()
s=(root/'test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
s=s.replace("out=root/'tracker-results'","out=root/'sprite-boundary/SMTests'").replace('out.mkdir(exist_ok=True)','out.mkdir(exist_ok=True,parents=True)')
exec(compile(s,'fixture','exec'))
for wide in (0,1):
    boot(str(wide)+'-'+uuid.uuid4().hex,False)
    assert l.sm_teleport(1)
    wait(440)
    assert l.sm_test_all_equipment()
    l.sm_set_widescreen(wide)
    wait(3)
    def hud():
        # Ignore the animated position marker in the minimap.
        data=C.string_at(l.sm_pixels(),256*240*4)
        return b''.join(data[y*1024:y*1024+208*4] for y in range(31))
    original=hud()
    for y in (260,272,288):
        put(0xafa,y);put(0xafc,0);put(0xb14,y)
        step()
        assert l.sm_samus_y()>=256,(wide,y,l.sm_samus_y())
        assert hud()==original,('Wrapped sprite changed HUD',wide,y)
        capture(f'below-{wide}-{y}')
    assert not l.sm_cpu_opcodes()
    l.sm_shutdown()
print('SPRITE_BELOW_ROOM_HUD_4X3_WIDE_PASS',flush=True)
