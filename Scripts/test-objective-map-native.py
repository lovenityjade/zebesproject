#!/usr/bin/env python3
"""Original source-writer icon ownership against native pixels and event gates."""
import os,sys,copy,itertools
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-objectives.py').read_text().split("for goal in audit['goals']:")[0]
source=source.replace('objectives/libsm_native.so','objectives-map/libsm_native.so').replace("root/'objectives/native'","root/'objectives-map/native'")
exec(compile(source,'objective-map-fixture','exec'))
from utils.objectives import Objectives
from graph.graph_utils import graphAreas,gameAreas
from rom.rompatcher import RomPatcher
from rom.rom import FakeROM
from rom.addresses import Addresses
mapaudit=json.loads((root/'objectives-map/map-audit.json').read_text())
render=routine('sm_objective_map_render',None,C.c_void_p)
draw_boss=routine('DrawMapIcons',None)
buf=(C.c_uint8*(256*240*4))();wide=base+symbols['sm_wide_hud'];overlay=l.sm_ui_overlay
cases=0;source_rows=[]

def writer(ids):
    Objectives.activeGoals=[]
    for rank,id in enumerate(ids):
        goal=copy.copy(Objectives.goals[audit['goals'][id]['name']]);goal.rank=rank+1;Objectives.activeGoals.append(goal)
    patcher=RomPatcher.__new__(RomPatcher);patcher.areaMaps={'Crateria':object()}
    patcher._accessibleAreasNoBoss=set(graphAreas);patcher._mapIconTableAddr=0x100000;patcher.romFile=FakeROM({})
    # Only the output pointer address is arbitrary. Run the unmodified source
    # membership/conflict writer and collect its area table pointers directly.
    pointers={};patcher._writeMapIconTable=lambda table,address:pointers.update(table)
    old=Addresses.getOne;Addresses.getOne=lambda name:0x180000
    try:patcher.writeObjectivesMapIcons('Vanilla')
    finally:Addresses.getOne=old
    rows=[]
    for area,address in pointers.items():
        while patcher.romFile.readWord(address)!=65535:
            rows.append([gameAreas.index(area)]+[patcher.romFile.readWord(address+2*i) for i in range(4)])
            address+=8
    return rows

def expected(width,rows):
    result=bytearray(width*240*4)
    if l.sm_state()!=15 or word(0x763)!=0 or not state(7):return bytes(result)
    area=l.sm_map_browser_state(3)
    for a,x,y,rank,subevent in rows:
        if a!=area or event(162+rank*2) or not subevent or event(subevent):continue
        icon=mapaudit['sprites'][rank]
        x+=icon['x']-C.c_int16(word(0xb1)).value+(width-256)//2
        y+=icon['y']-C.c_int16(word(0xb3)).value
        for n,c in enumerate(icon['pixels']):
            px,py=x+n%8,y+n//8
            if c==65535 or not (8<=px<width-8 and 48<=py<192):continue
            offset=(py*width+px)*4
            result[offset:offset+4]=bytes([((v<<3)|(v>>2)) for v in [(c>>10)&31,(c>>5)&31,c&31]]+[255])
    return bytes(result)

def verify(label,rows):
    global cases
    C.memset(buf,0,len(buf));C.memset(wide,0,400*240*4);C.memset(overlay(),0,256*240*4)
    before=read(0,131072);render(buf);assert read(0,131072)==before,('Render changed RAM',label)
    for width,actual in [(256,bytes(buf)),(256,C.string_at(overlay(),256*240*4)),(400,C.string_at(wide,400*240*4))]:
        wanted=expected(width,rows)
        assert actual==wanted,(label,width,[(i,a,b) for i,(a,b) in enumerate(zip(actual,wanted)) if a!=b][:12])
    cases+=1

def setup(ids,flags=0):
    commit(plan(ids,flags=flags));clear();put(0x998,15);put(0x763,0)
    return writer(ids)
def center(row):
    area,x,y,*_=row;put(0x79f,area);put(0xb1,(x-128)&65535);put(0xb3,(y-120)&65535)

for goal in audit['goals']:
    if not goal['supported']:continue
    ids=[goal['id']];rows=setup(ids);source_rows.append(dict(goals=ids,rows=rows))
    for row in rows:
        clear();put(0x998,15);put(0x763,0);center(row)
        verify(('unknown-map-visible',goal['name'],row),rows)
        mark(row[4]);verify(('subevent-complete',goal['name'],row),rows)
        mark(162);verify(('goal-complete',goal['name'],row),rows)
    rows=setup(ids,2);verify(('hidden',goal['name']),rows)
    mark(161);verify(('revealed',goal['name']),rows)

# Source overlap policy in both orders. These raw native boundary contracts
# test the writer's ownership rule, independently of request exclusions.
conflicts=[['kill two G4','kill kraid'],['kill two minibosses','kill golden torizo'],
           ['activate chozo robots','kill golden torizo'],['kill king cacatac','kill all cacatacs']]
for pair in conflicts:
    for names in [pair,list(reversed(pair))]:
        ids=[byname[n] for n in names];rows=setup(ids);source_rows.append(dict(goals=ids,rows=rows))
        for row in rows:
            center(row);verify(('conflict',names,row),rows)
        for rank in range(2):
            mark(162+rank*2)
            for row in rows:center(row);verify(('completed-owner-no-fallback',names,rank,row),rows)

no_icons=[g['id'] for g in mapaudit['goals'] if not g['points'] and audit['goals'][g['id']]['supported']][:17]
fish=byname['tickle the red fish']
for rank in range(18):
    ids=no_icons.copy();ids.insert(rank,fish);rows=setup(ids);assert len(rows)==1 and rows[0][3]==rank
    center(rows[0]);verify(('rank',rank+1),rows)
    for sx,sy in [(7,48),(252,48),(128,45),(128,191)]:
        put(0xb1,(rows[0][1]-sx)&65535);put(0xb3,(rows[0][2]-sy)&65535);verify(('clip',rank,sx,sy),rows)
    put(0x998,8);verify(('no-gameplay-map-overlay',rank),rows)
    put(0x998,15);put(0x763,1);verify(('no-equipment-overlay',rank),rows)
    put(0x763,2);verify(('no-objectives-page-overlay',rank),rows)

# Original boss boxes are suppressed only for an active objective contract.
rows=setup([byname['kill kraid']]);put(0x79f,1);put(0xb1,320);put(0xb3,64)
put(0x789,1)
put(0x590,0);draw_boss();with_objectives=word(0x590)
pause_ram=read(0,131072)
assert config(0,None,audit['sha256'].encode()) and activate(0,1) and select(items,100,m['sha256'].encode())
C.memmove(ram,pause_ram,len(pause_ram));put(0x590,0);draw_boss();without_objectives=word(0x590)
assert without_objectives>with_objectives,(without_objectives,with_objectives)
assert activate(0,0) and select(None,0,None)
verify('vanilla-no-objective-overlay',rows)
assert l.sm_cpu_opcodes()==0
(root/'objectives-map/map-results.json').write_text(json.dumps(dict(passed=True,cases=cases,sourceWriter=source_rows,
    ranks=18,hidden=True,subevents=True,completion=True,conflictPriority=True,clipping=True,vanillaBossBoxes=True,
    cpuOpcodes=0,nativeSha256=hashlib.sha256((root/'objectives-map/libsm_native.so').read_bytes()).hexdigest()),indent=2)+'\n')
l.sm_shutdown();print('OBJECTIVE_MAP_NATIVE_PASS',cases,flush=True)
