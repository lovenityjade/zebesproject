#!/usr/bin/env python3
"""Native HUD capture in an initialized gameplay room, gaming-pc only."""
import sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
source=(root/'test-elevators-hud.py').read_text().split('results=[]')[0]
exec(compile(source,'native-fixture','exec'))
m=json.loads((root/'elevators-hud/seed-3.json').read_text())['manifest']
boot('hud-render-'+uuid.uuid4().hex,True)
l.sm_seed_rules_configure(32);l.sm_varia_ui_configure(15)
counts=(C.c_uint8*100)(*[p['hudCounted'] for p in sorted(m['placements'],key=lambda p:p['address'])])
l.sm_varia_ui_counted_configure(counts,100)
wait(2);capture('full-with-hud')
result=dict(seed=m['seed'],region=l.sm_varia_ui_state(1),remaining=l.sm_varia_ui_state(2),cpu=l.sm_cpu_opcodes())
assert result['cpu']==0
(out/'render.json').write_text(json.dumps(result,indent=2));l.sm_shutdown();print('HUD_RENDER_PASS',result,flush=True)
