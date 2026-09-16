#!/usr/bin/env python3
"""Exercise managed-slot route copies/archives with isolated native gameplay."""
import sys, uuid
from pathlib import Path

root = Path(sys.argv[1]).resolve()
fixture = (root / 'test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
fixture = fixture.replace("out=root/'tracker-results'", "out=root/'route-slots/SMTests'")
fixture = fixture.replace('out.mkdir(exist_ok=True)', 'out.mkdir(exist_ok=True,parents=True)')
exec(compile(fixture, 'fixture', 'exec'))
label = uuid.uuid4().hex
boot(label, True)
seed = m['sha256']
save = out / label / seed / 'sram.dat'
items = (Item * 100)(*[Item(i['address'], i['plm']) for i in m['placements']])
l.sm_slots_set.argtypes = [C.c_int, C.c_int, C.c_int, C.POINTER(Item), C.c_int, C.c_char_p]
callback_type = C.CFUNCTYPE(C.c_int, C.c_int, C.c_int, C.c_int, C.c_void_p)
reject = False

@callback_type
def callback(action, source, other, context):
    if reject:
        return 0
    if action == 2:
        return l.sm_slots_set(other, 1, 1, items, 100, seed.encode())
    if action == 3:
        return l.sm_slots_set(source, 0, 0, None, 0, b'')
    return 0

l.sm_slots_enable(callback, None)
for slot in range(3):
    assert l.sm_slots_set(slot, 1, 1, items, 100, seed.encode())
assert l.sm_test_playtest(8, 1)
for buttons, frames in [(128, 80), (64, 90), (128, 80)]:
    for _ in range(frames):
        step(buttons)
assert l.sm_save()
source = Path(f'{save}.route-1-{seed}')
destination = Path(f'{save}.route-0-{seed}')
source_bytes = source.read_bytes()
destination.write_bytes(b'old destination history')
reject = True
assert not l.sm_test_playtest(5, 0)
assert destination.read_bytes() == b'old destination history'
reject = False
assert l.sm_test_playtest(5, 0)
assert destination.read_bytes() == source.read_bytes() == source_bytes
archives = list(destination.parent.glob(destination.name + '.archive-*'))
assert len(archives) == 1 and archives[0].read_bytes() == b'old destination history'
count = l.sm_route_state(0)
assert l.sm_test_playtest(8, 0)
step()
assert l.sm_route_state(0) == count + 1 and not l.sm_route_state(4)
assert l.sm_save()
copied = destination.read_bytes()
assert l.sm_test_playtest(6, 0)
assert not destination.exists() and l.sm_route_state(0) == 0
assert any(p.read_bytes() == copied for p in destination.parent.glob(destination.name + '.archive-*'))
assert source.read_bytes() == source_bytes
# Reusing the same seed after clearing must start a separate history.
assert l.sm_slots_set(0, 1, 1, items, 100, seed.encode())
assert l.sm_test_playtest(8, 0)
step()
assert l.sm_route_state(0) == 1
assert l.sm_save()
assert destination.read_bytes() != copied
l.sm_shutdown()
assert hashlib.sha256(rom.read_bytes()).hexdigest() == rom_hash
print('ROUTE_SLOT_COPY_REJECT_CLEAR_ARCHIVE_REUSE_PASS', flush=True)
