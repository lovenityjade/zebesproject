#!/usr/bin/env python3
"""Original VARIA sprite pixels, visibility and committed-world map rendering.

Controlled native pause/minimap fixtures on gaming-pc, not manual traversal.
"""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-area-connections.py').read_text().split('cases=[];visited=set()')[0]
source=source.replace("root/'areas/native'","root/'map-icons/native'").replace('areas/libsm_native.so','map-icons/libsm_native.so')
exec(compile(source,'map-icon-fixture','exec'))
audit=json.loads((root/'map-icons/audit.json').read_text())
sprites=audit['icons'];door_catalog=json.loads((root/'Randomizer/native_door_colors.json').read_text())
render=routine('sm_map_icons_render',None,C.c_void_p)
doors_activate=routine('sm_doors_activate',None,C.c_int)
boss_activate=routine('sm_connections_activate',None,C.c_int)
l.sm_doors_configure.argtypes=l.sm_areas_configure.argtypes
l.sm_connections_configure.argtypes=l.sm_areas_configure.argtypes
l.sm_doors_catalog_sha256.restype=l.sm_connections_catalog_sha256.restype=C.c_char_p
buf=(C.c_uint8*(256*240*4))();wide=base+symbols['sm_wide_hud'];ui=l.sm_ui_overlay;ui.restype=C.c_void_p
door_values=[0]*58;area_values=[];boss_values=[];queries=0
def commit():
    assert l.sm_doors_configure(0,(C.c_uint8*58)(*door_values),58,l.sm_doors_catalog_sha256())
    assert configure(0,area_values)
    assert l.sm_connections_configure(0,(C.c_uint8*len(boss_values))(*boss_values),len(boss_values),l.sm_connections_catalog_sha256())
    area_activate(0);doors_activate(0);boss_activate(0)
    assert select(items,100,m['sha256'].encode())

def clear(area=0,pause=True):
    C.memmove(ram,baseline,len(baseline));put(0x998,15 if pause else 8);put(0x79f,area);put(0x763,0)
    C.memset(ram+0x7f7,0,256);C.memset(ram+0xcd52,0,2048);C.memset(ram+0xd908,0,8);C.memset(ram+0xd8b0,0,64)
    put(0xb1,0);put(0xb3,0);l.sm_seed_rules_configure(4)

def explore(p):
    offset=(0x7f7 if p['area']==word(0x79f) else 0xcd52+256*p['area'])+p['byte']
    C.c_uint8.from_address(ram+offset).value|=p['mask']

def visible(p):
    offset=(0x7f7 if p['area']==word(0x79f) else 0xcd52+256*p['area'])+p['byte']
    return bool(read(offset,1)[0]&p['mask'])

def expected(width):
    result=bytearray(width*240*4);pause=word(0x998)==15
    def stamp(p,sprite):
        if p['area']!=word(0x79f):return
        if pause:
            x=p['x']*8-C.c_int16(word(0xb1)).value+(width-256)//2;y=p['y']*8-C.c_int16(word(0xb3)).value
            clip=(8,48,width-8,192)
        else:
            mx=word(0x7a1)+(word(0xaf6)>>8);my=word(0x7a3)+(word(0xafa)>>8)+1
            if (p['x'],p['y'])==(mx,my):return
            left=336 if width==400 else 208;columns=7 if width==400 else 5
            x=left+(p['x']-mx+columns-3)*8;y=(p['y']-my+1)*8
            clip=(left,0,left+columns*8,32 if width==400 else 24)
        icon=sprites[sprite];x+=icon['x'];y+=icon['y']
        for n,c in enumerate(icon['pixels']):
            px=x+n%8;py=y+n//8
            if c==65535 or not (clip[0]<=px<clip[2] and clip[1]<=py<clip[3]):continue
            channels=[(c>>10)&31,(c>>5)&31,c&31]
            offset=(py*width+px)*4
            result[offset:offset+4]=bytes([(v<<3)|(v>>2) for v in channels]+[255])
    for d in audit['doors']:
        color=door_values[d['id']];bit=d['openedBit']
        if color and visible(d['position']) and not read(0xd8b0+bit//8,1)[0]&(1<<(bit%8)):
            stamp(d['position'],audit['doorIcons'][color][d['facing']])
    for domain,values in [('area',area_values),('boss',boss_values)]:
        for i,j in enumerate(values):
            entries=audit['domains'][domain];p=entries[i]['display']
            if not visible(p):
                if not l.sm_seed_rules_active()&4 or read(0xd908+p['area'],1)[0]:stamp(p,audit['unexploredIcon'])
            elif visible(entries[j]['actual']):stamp(p,entries[j]['destinationIcon'])
    return bytes(result)

def verify(label):
    global queries
    C.memset(buf,0,len(buf));C.memset(wide,0,400*240*4);C.memset(ui(),0,256*240*4)
    before=read(0,131072);render(buf);assert before==read(0,131072),('Render changed gameplay RAM',label)
    for width,actual in [(256,bytes(buf)),(256,C.string_at(ui(),256*240*4)),(400,C.string_at(wide,400*240*4))]:
        want=expected(width)
        if actual!=want:
            differences=[(i,a,b) for i,(a,b) in enumerate(zip(actual,want)) if a!=b]
            raise AssertionError((label,width,len(differences),differences[:12]))
    queries+=1

commit();door_cases=[]
for d in audit['doors']:
    p=d['position']
    for color in range(1,9):
        door_values=[0]*58;door_values[d['id']]=color;commit();clear(p['area'])
        put(0xb1,(p['x']*8-128)&65535);put(0xb3,(p['y']*8-120)&65535)
        verify(('door-unexplored',d['name'],color))
        explore(p);verify(('door-closed',d['name'],color))
        bit=d['openedBit'];C.c_uint8.from_address(ram+0xd8b0+bit//8).value|=1<<(bit%8)
        verify(('door-open',d['name'],color))
        door_cases.append((d['name'],color))
door_values=[0]*58
for offset in range(32):
    area_values=[(offset-i)%32 for i in range(32)];commit()
    for i,j in enumerate(area_values):
        entries=audit['domains']['area'];p=entries[i]['display'];clear(p['area'])
        put(0xb1,(p['x']*8-128)&65535);put(0xb3,(p['y']*8-120)&65535)
        explore(p);verify(('source-only',i,j))
        explore(entries[j]['actual']);verify(('both-ends',i,j))
area_values=[]
for shift in range(4):
    boss_values=[0]*8
    for outside in range(4):
        inside=(outside+shift)%4*2;boss_values[outside*2+1]=inside;boss_values[inside]=outside*2+1
    commit()
    for i,j in enumerate(boss_values):
        entries=audit['domains']['boss'];p=entries[i]['display'];clear(p['area'])
        put(0xb1,(p['x']*8-128)&65535);put(0xb3,(p['y']*8-120)&65535)
        explore(p);explore(entries[j]['actual']);verify(('boss',i,j))
boss_values=[];area_values=[(1-i)%32 for i in range(32)];commit()
for area in range(6):
    clear(area);C.c_uint8.from_address(ram+0xd908+area).value=1;verify(('map-station-unknown',area))
    C.c_uint8.from_address(ram+0xd908+area).value=0;l.sm_seed_rules_configure(0);verify(('full-map-unknown',area))
    C.memset(ram+0x7f7,255,256);C.memset(ram+0xcd52,255,2048)
    for x,y in [(0,0),(32,48),(128,96),(320,128)]:
        put(0xb1,x);put(0xb3,y);verify(('clipping',area,x,y))
    capture('pause-area-'+str(area))
    put(0x998,8);put(0x7a1,0);put(0x7a3,0)
    for p in [a['display'] for a in audit['domains']['area'] if a['display']['area']==area]:
        put(0xaf6,p['x']*256);put(0xafa,(p['y']-1)*256);verify(('cursor-protected',area,p))
        put(0xaf6,max(0,p['x']-1)*256);verify(('minimap',area,p))

# Combined area/boss/color rendering uses the same layer order as the native
# map, with checks painted later by the independent tracker renderer.
boss_values=[1,0,3,2,5,4,7,6]
door_values=[(i%8)+1 if d['canRandom'] else 0 for i,d in enumerate(door_catalog['locations'])]
commit()
for area in range(6):
    clear(area);C.memset(ram+0x7f7,255,256);C.memset(ram+0xcd52,255,2048)
    put(0xb1,160);put(0xb3,48);verify(('combined-world',area))

# Pending configuration does not replace the committed map. Failed item-table
# transactions preserve its icons too. Vanilla renders no VARIA icons.
assert configure(0,[(7-i)%32 for i in range(32)])
verify('pending-world')
bad=(Item*100)(*items);bad[0].address=0
assert not select(bad,100,m['sha256'].encode());verify('rejected-world')
assert select(None,0,None)
C.memset(buf,0,len(buf));C.memset(wide,0,400*240*4);render(buf)
assert not any(buf) and not any(C.string_at(wide,400*240*4))
# Seeds without shuffled door colors use their actual restored PLM colors.
# This also checks returning from a combined world to ordinary topology.
area_values=[];boss_values=[];door_values=[0]*58;commit()
assert l.sm_doors_configure(0,None,0,l.sm_doors_catalog_sha256())
doors_activate(0);assert select(items,100,m['sha256'].encode())
door_values=[door_catalog['colors'].index(d['vanillaColor']) if d['canRandom'] else 0 for d in door_catalog['locations']]
for area in range(6):
    clear(area);C.memset(ram+0x7f7,255,256);put(0xb1,160);put(0xb3,48);verify(('original-door-requirements',area))
# The visual-test helper only accepts gameplay and a Boolean fixture state.
assert not l.sm_test_map_explored(2)
put(0x998,15);assert not l.sm_test_map_explored(1)
put(0x998,8);assert l.sm_test_map_explored(1)
assert read(0x7f7,256)==b'\xff'*256 and read(0xcd52,2048)==b'\xff'*2048
assert l.sm_test_map_explored(0) and read(0x7f7,256)==bytes(256) and read(0xcd52,2048)==bytes(2048)
assert l.sm_cpu_opcodes()==0;l.sm_shutdown()
assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(out/'verification.json').write_text(json.dumps(dict(passed=True,queries=queries,doorCases=len(door_cases),
    directedAreaPairs=1024,directedBossPairs=32,vanillaUntouched=True,committedWorldOnly=True,renderDoesNotMutateRam=True,
    nativeSha256=hashlib.sha256(libpath.read_bytes()).hexdigest(),cpu=0,sourceRomUnchanged=True,scope=__doc__),indent=2)+'\n')
print('NATIVE_MAP_ICONS_PASS',queries,flush=True)
