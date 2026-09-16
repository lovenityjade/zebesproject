import sys
from pathlib import Path
root=Path(sys.argv[1])
s=(root/'test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
s=s.replace("('plm',C.c_uint16)]","('plm',C.c_uint16),('kind',C.c_uint16)]")
s=s.replace("root/'Native/build/libsm_native.so'","root/'full-options/libsm_native.so'").replace("root/'results/seed-14092026.json'","root/'full-options/relic.json'")
s=s.replace("Item(i['address'],i['plm'])","Item(i['address'],i['plm'],i.get('kind',0))")
s=s.replace("out=root/'tracker-results'","out=root/'full-options/SMTests'")
s=s.replace('ram=l.sm_simulation_ram();l.sm_set_widescreen(1)', '''ram=l.sm_simulation_ram();l.sm_set_widescreen(1)
 l.sm_slots_enable(None,None)
 for slot in range(3):assert l.sm_slots_set(slot,1,1,items,100,m['sha256'].encode())
 l.sm_relic_configure(1,20)''')
exec(compile(s,'fixture','exec'))
# Force a visible relic at the original Morph location, preserving the native location ID.
for p in m['placements']:
 if p['location']=='Morphing Ball':p.update(item='ChozoRelic',kind=1,plm=0xef27)
boot('relic-visual',True)
l.sm_tracker_configure(1,1,1)
assert l.sm_relic_required()==20
assert l.sm_test_room(0x9e9f,69*16+40,41*16+8)
for _ in range(2000):
 step()
 if l.sm_state()==8 and l.sm_room()==0x9e9f:break
wait(440)
assert l.sm_test_seed_pickup_position()
wait(20)
capture('relic-before')
before=bytes(snap());b=snap();count=l.sm_relic_count()
assert l.sm_test_seed_pickup_position()
events=[]
for _ in range(100):
 step(64)
 events += [l.sm_visual_state(17,i) for i in range(l.sm_visual_state(14,0))]
 if l.sm_relic_count()>count:break
assert l.sm_message_active()
wait(40);capture('relic-message')
for i in range(600):
 step(256 if i>380 else 0)
 events += [l.sm_visual_state(17,j) for j in range(l.sm_visual_state(14,0))]
 if not l.sm_message_active():break
capture('relic-collected')
a=snap()
assert l.sm_relic_count()==count+1,(count,l.sm_relic_count())
for field in ['max_reserve','max_health','max_missiles','max_supers','max_power_bombs','acquired_items','acquired_beams']:
 assert getattr(a,field)==getattr(b,field),(field,getattr(b,field),getattr(a,field))
assert 140 in events,events
assert not l.sm_message_active()
assert l.sm_save()
press(8)
for _ in range(1000):
 step()
 if l.sm_state()==15:break
wait(80);capture('relic-counter')
assert l.sm_cpu_opcodes()==0
(out/'result.json').write_text(json.dumps(dict(collected=l.sm_relic_count(),required=l.sm_relic_required(),unchangedInventory=True,events=events,emulatedCpu=0),indent=2))
l.sm_shutdown()
print('RELIC_RUNTIME_PASS',flush=True)
