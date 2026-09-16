#!/usr/bin/env python3
"""Native VARIA event identity, repeat deaths, exact AI calls and original SRAM."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-area-connections.py').read_text().split('cases=[];visited=set()')[0]
source=source.replace("root/'areas/native'","root/'objective-events/native'").replace('areas/libsm_native.so','objective-events/libsm_native.so')
exec(compile(source,'objective-events-fixture','exec'))
audit=json.loads((root/'objective-events/events-audit.json').read_text());events=audit['events']
assert select(items,100,m['sha256'].encode())
mark=routine('sm_objective_event_mark',None,C.c_uint)
properties=routine('sm_objective_enemy_properties',C.c_uint16,C.c_uint,C.c_uint,C.c_uint16)
death=routine('EnemyDeathAnimation',None,C.c_uint16,C.c_uint16)
update=routine('sm_objective_events_frame',None)
original_ai=routine('sm_original_CallEnemyAi',None,C.c_uint32);ai=routine('CallEnemyAi',None,C.c_uint32)
get=l.sm_objective_event;get.argtypes=[C.c_uint]
count=l.sm_objective_enemy_count;count.argtypes=[C.c_int,C.c_int]
save=routine('SaveToSram',None,C.c_uint16);load=routine('LoadFromSram',C.c_uint8,C.c_uint16)
results=[];extension=0xd91c+0x180;length=4+audit['eventBytes']
def clear():
    C.memmove(ram,baseline,len(baseline));put(0x998,8)
    C.memset(ram+extension,0,length)
def enemy(p,index=0):
    put(0xe54,index);C.memset(ram+0xf78+index,0,64)
    put(0xf78+index,p['enemy']);put(0xf88+index,properties(0xa1,p['population'],p['original']))
for p in audit['enemies']:
    assert properties(0xa1,p['population'],p['original'])==p['properties']
    assert properties(0xa2,p['population'],p['original'])==p['original']
clear();killed=set()
for p in audit['enemies']:
    enemy(p);death(0,0);killed.add(p['event']);assert get(p['event'])
    for kind in range(6):
        assert count(kind,0)==sum(e['event'] in killed and e['type']==kind for e in audit['enemies'])
    members={e['event'] for e in audit['enemies'] if e['roomEvent']==p['roomEvent']}
    assert bool(get(p['roomEvent']))==(members<=killed)
    before=read(extension,length);enemy(p);death(0,0);assert before==read(extension,length)
assert [count(i,0) for i in range(6)]==[count(i,1) for i in range(6)]
for name in ['space_pirates','ki_hunters','beetoms','cacatacs','kagos','yapping_maws']:assert get(events[name+'_all_event'])
results.append(dict(kind='original_death_routine',uniqueEnemies=len(killed),repeatedDeaths=len(killed),totals=[count(i,0) for i in range(6)]))
# Source checks after the exact original AI call; gameplay RAM must match the
# original call except the dedicated versioned event extension.
cases=[]
for spec in audit['ai']:
    for room in [0xd104 if spec['kind']=='fish' else 0xacb3,0x91f8]:
        for health in [0,100]:
            clear();put(0x79b,room);put(0xe54,0);C.memset(ram+0xf78,0,64)
            put(0xf78,spec['enemy']);put(0xf8c,health);put(0x18a6,0);put(0xdc4,0)
            before=read(0,131072);original_ai(spec['target']);expected=bytearray(read(0,131072))
            event=None
            if spec['kind']=='fish' and room==0xd104:event='fish_tickled_event'
            elif spec['kind']=='geemer' and word(0xf8c)==0:event='orange_geemer_event'
            elif spec['kind']=='shaktool' and word(0xf8c)==0:event='shak_dead_event'
            elif spec['kind']=='cacatac' and room==0xacb3 and word(0xf8c)==0:event='king_cac_event'
            C.memmove(ram,before,len(before));ai(spec['target']);actual=bytearray(read(0,131072))
            assert bool(get(events[event])) if event else actual[extension:extension+length]==expected[extension:extension+length]
            actual[extension:extension+length]=expected[extension:extension+length]
            assert actual==expected,(spec,room,health,[(hex(i),a,b) for i,(a,b) in enumerate(zip(actual,expected)) if a!=b][:10])
            cases.append(dict(kind=spec['kind'],target=spec['target'],room=room,health=health,event=event))
results.append(dict(kind='original_ai_parity',cases=cases))
# Native bowling animation instruction; original world effects still run.
clear();args=(C.c_uint16*1)(0);bowling=routine('Shaktool_Instr_14',C.c_void_p,C.c_uint16,C.c_void_p)
assert bowling(0,C.addressof(args))==C.addressof(args);assert get(events['bowling_chozo_event'])
# Animals use real explored bits, not fully-known map data, only in Brinstar.
clear();put(0x79f,0);put(0x828,0x10);put(0x82c,0x20);update();assert not get(events['etecoons_event'])
put(0x79f,1);update();assert get(events['etecoons_event']) and get(events['dachora_event'])
results.append(dict(kind='bowling_and_animals',passed=True))
# A/B/C and historical files. Event storage is inside the native checksummed
# save block and disjoint from ZME1 and original map packing.
patterns=[]
for slot in range(3):
    clear()
    for p in audit['enemies'][slot*5:slot*5+slot+1]:mark(p['event'])
    mark(events['fish_tickled_event']+slot)
    patterns.append(read(extension,length));save(slot)
for slot in [2,0,1,0,2,1]:
    C.memset(ram+extension,0,length);assert load(slot)==0
    assert read(extension,length)==patterns[slot]
results.append(dict(kind='native_sram',slots=3,loads=6))
clear();C.memset(ram+extension,0x55,length);assert not any(get(p['event']) for p in audit['enemies'])
mark(audit['enemies'][0]['event']);assert count(0,0)==1
# Original boss events remain sourced from actual native boss bits.
clear();put(0xd829,1);before=read(0,131072)
assert get(events['kraid_event']) and not get(events['ridley_event'])
assert read(0,131072)==before
# Membership follows the source extra-property tag, even if an AI changes its header.
clear();p=audit['enemies'][0];enemy(p);put(0xf78,0xdaff)
routine('sm_objective_enemy_death',None,C.c_uint)(0);assert get(p['event'])
# Real room loader populates the source-defined properties before enemy AI.
clear();put(0xd820,1)  # Zebes awake: the source Climb pirate population.
assert l.sm_test_all_equipment()  # Keep the unattended room-load fixture alive.
assert l.sm_test_room(0x96ba,128,128)
for n in range(2000):
    step()
    if l.sm_state()==8 and l.sm_room()==0x96ba:break
else:raise AssertionError('Climb room load')
loaded=[]
for i in range(32):
    prop=word(0xf88+i*64)
    if not prop&0x4000:continue
    index=(prop&0x3ff8)>>3;p=audit['enemies'][index]
    assert word(0xf78+i*64)==p['enemy'];loaded.append(index)
assert len(loaded)==11,loaded
results.append(dict(kind='real_climb_room_population',enemies=loaded))
# Real room population and real AI entry, without substituting an enemy header.
wait(440);assert l.sm_test_room(0xd104,128,128),(l.sm_state(),hex(l.sm_room()),word(0x9c2))
for n in range(2000):
    step()
    if l.sm_state()==8 and l.sm_room()==0xd104:break
else:raise AssertionError('Red Fish load')
fish=[i*64 for i in range(32) if word(0xf78+i*64)==0xd6ff];assert fish
put(0xe54,fish[0]);ai(0xa38000);assert get(events['fish_tickled_event'])
wait(440);assert l.sm_test_room(0xacb3,128,128)
for n in range(2000):
    step()
    if l.sm_state()==8 and l.sm_room()==0xacb3:break
else:raise AssertionError('Bubble Mountain load')
cacti=[i*64 for i in range(32) if word(0xf78+i*64)==0xcfff];assert cacti
put(0xe54,cacti[0]);put(0xa6e,3)  # Original Speed Booster contact damage.
ai(0xa28023);assert get(events['king_cac_event'])
results.append(dict(kind='real_special_enemies',fishIndices=fish,cacatacIndices=cacti))

# Vanilla is untouched, even with seeded data sitting in the save buffer.
assert select(None,0,None);before=read(0,131072);mark(130);update();assert read(0,131072)==before
for p in audit['enemies']:assert properties(0xa1,p['population'],p['original'])==p['original']
assert count(0,0)==-1 and not get(130)
assert l.sm_cpu_opcodes()==0;l.sm_shutdown();assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(root/'objective-events/events-results.json').write_text(json.dumps(dict(passed=True,results=results,nativeSha256=hashlib.sha256(libpath.read_bytes()).hexdigest(),cpu=0),indent=2)+'\n')
print('OBJECTIVE_EVENTS_NATIVE_PASS',len(results),'groups')
