#!/usr/bin/env python3
"""Real statue-room loader, animation and passage with non-boss objectives."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-objectives.py').read_text().split("for goal in audit['goals']:")[0]
exec(compile(source,'objectives-tourian-fixture','exec'))
results=[]
for completed,bosses in [(False,True),(True,False)]:
    p=plan([byname['tickle the red fish']],flags=2);commit(p);clear()
    if bosses:
        for e in [72,88,96,80]:mark(e)
    if completed:mark(130)
    frame();assert state(4)==int(completed)
    assert l.sm_test_room(0xa66a,48,96)
    for _ in range(2500):
        step()
        if l.sm_state()==8 and l.sm_room()==0xa66a:break
    else:raise AssertionError('Statues Room load')
    # Original statue animation runs long enough for all four defeated bosses.
    wait(2400)
    assert l.sm_room()==0xa66a and l.sm_state()==8
    assert state(4)==int(completed) and state(7)==1
    if not completed:assert not any(event(i) for i in [6,7,8,9,10])
    else:assert not any(event(i) for i in [72,88,96,80])
    floor=[word(0x10002+2*(12*16+x)) for x in range(6,10)]
    scroll=word(0xcd20)
    assert scroll==(0x202 if completed else 1),(completed,hex(scroll))
    # Four original solid floor blocks become air when the configured quota opens Tourian.
    assert all((tile>>12)==(0 if completed else 8) for tile in floor),(completed,floor)
    label='objective-open-no-bosses' if completed else 'objective-locked-all-bosses'
    capture(label)
    results.append(dict(completed=completed,bosses=bosses,floor=floor,scroll=scroll,frameState=l.sm_state()))
assert l.sm_cpu_opcodes()==0
assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
l.sm_shutdown()
(root/'objectives/tourian-results.json').write_text(json.dumps(dict(cases=results,nativeSha256=hashlib.sha256((root/'objectives/libsm_native.so').read_bytes()).hexdigest()),indent=2)+'\n')
print('OBJECTIVES_TOURIAN_PASS',flush=True)
