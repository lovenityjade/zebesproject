#!/usr/bin/env python3
"""Compile VARIA event identities and original enemy population membership."""
import hashlib,json,re,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];up=root/'Randomizer/upstream';sys.path.insert(0,str(up))
from logic.logic import Logic
Logic.factory('vanilla')
from utils.objectives import Objectives
from rom.rom import snes_to_pc
from rom.enemies_objectives_data import enemies_objectives_data
src=up/'patches/common/src';constants={}
sources=[src/'include/event_list.asm',src/'include/enemies_events.asm',src/'objectives/enemies.asm',src/'objectives.asm',up/'utils/objectives.py',up/'rom/enemies_objectives_data.py']
for path in sources[:2]:
    for name,expr in re.findall(r'^!(\w+)\s*#=\s*([^;\n]+)',path.read_text(),re.M):
        expr=re.sub(r'\$([0-9a-fA-F]+)',r'0x\1',expr.strip())
        try:expr=re.sub(r'!(\w+)',lambda m:str(constants[m[1]]),expr)
        except KeyError:continue
        if re.fullmatch(r'[0-9a-fx+* ()]+',expr):constants[name]=eval(expr,{'__builtins__':{}},{})
rom=(root/'roms/Super Metroid (Japan, USA) (En,Ja).sfc').read_bytes()
assert hashlib.sha256(rom).hexdigest()=='12b77c4bc9c1832cee8881244659065ee1d84c70c3d29e6eaf92e6798cc2ca72'
asm=sources[2].read_text();rows=[]
pattern=r'dw !(\w+), (\w+)_type, (\w+)_events, 0\s+pushpc\s+org \$([a-f0-9]+)\s+dw \$([a-f0-9]+)\s+pullpc'
for event,kind,room,address,value in re.findall(pattern,asm):
    address=int(address,16);value=int(value,16);offset=snes_to_pc(address)
    original=int.from_bytes(rom[offset:offset+2],'little')
    assert (value&0x3ff8)>>3==len(rows) and value&0x4000
    assert original==(value&7) and original&0x7ff8==0
    rows.append(dict(id=len(rows),event=constants[event],name=event,type=constants[kind+'_type_index'],roomEvent=constants[room+'_all_event'],population=(address&65535)-10,properties=value,original=original,enemy=int.from_bytes(rom[offset-10:offset-8],'little')))
assert len(rows)==sum(sum(t['area_count'].values()) for t in enemies_objectives_data.values())
assert len({r['event'] for r in rows})==len(rows) and len({r['population'] for r in rows})==len(rows)
events={n:v for n,v in constants.items() if n.endswith('_event')};last=max(events.values());size=(last-128+8)//8
assert size<=64 and min(r['event'] for r in rows)>=198
goals=[dict(id=i,name=name,symbol=g.symbol,inProgress=g.inProgressSymbol,available=g.available,category=g.gtype,area=g.area) for i,(name,g) in enumerate(Objectives.goals.items())]
# Original AI fields patched by objectives.asm. Compile their original target
# addresses so the C dispatcher can observe those exact calls without ROM code.
ai=[]
for name,addresses in [('fish',[0xd719]),('geemer',[0xdc59,0xdc67,0xdc6f,0xdc71]),('shaktool',[0xf0af,0xf0b1]),('cacatac',[0xd019,0xd027,0xd02f,0xd031])]:
    # EnemyDef.grapple_ai offset $1a; touch_ai offset $30. Derive the header
    # from the actual patched field rather than a nearby similar enemy.
    header=addresses[0]-(0x30 if name=='shaktool' else 0x1a)
    bank=rom[snes_to_pc(0xa00000|header)+12]
    # Verify identity independently against the native decompilation's init entry.
    expected={'fish':0xa390b5,'geemer':0xa3e043,'shaktool':0xaade43,'cacatac':0xa29f48}[name]
    offset=snes_to_pc(0xa00000|header)+18
    assert bank<<16|int.from_bytes(rom[offset:offset+2],'little')==expected
    # The enemy header's AI bank is offset 12; addresses point to its AI words.
    for a in addresses:
        target=int.from_bytes(rom[snes_to_pc(0xa00000|a):snes_to_pc(0xa00000|a)+2],'little')
        # Original PowerBombReaction falls back to bank:$8037 for a zero AI word.
        if target==0:
            assert a in [0xdc67,0xd027];target=0x8037
        ai.append(dict(kind=name,enemy=header,field=a,target=bank<<16|target))
audit=dict(schema=1,eventBase=128,eventBytes=size,events=events,enemies=rows,goals=goals,ai=ai,
    sources={str(p.relative_to(up)):hashlib.sha256(p.read_bytes()).hexdigest() for p in sources})
# ZOE1 is persistent. Reordering upstream event or population identities must
# require an explicit format migration rather than reinterpreting old saves.
identity=dict(format='ZOE1',eventBase=128,eventBytes=size,events=events,
    enemies=[{k:r[k] for k in ['population','event','type','roomEvent']} for r in rows])
identity['sha256']=hashlib.sha256(json.dumps(identity,sort_keys=True,separators=(',',':')).encode()).hexdigest()
lock=root/'Randomizer/native_objective_events.json'
if lock.exists():assert json.loads(lock.read_text())==identity,'Objective event identities changed: migrate/version ZOE1 before rebuilding'
else:lock.write_text(json.dumps(identity,indent=2)+'\n')
audit['persistentCatalogSha256']=identity['sha256']
out=root/'Docs/Randomizer/FullOptions/Objectives';out.mkdir(exist_ok=True)
(out/'events-audit.json').write_text(json.dumps(audit,indent=2)+'\n')
lines=['/* Generated source event/population identities; no emulated code. */',f'#define OBJECTIVE_EVENT_BYTES {size}',f'#define OBJECTIVE_EVENT_LAST {last}']
for n in ['fish_tickled','orange_geemer','shak_dead','bowling_chozo','king_cac','etecoons','dachora']:
    lines.append(f'#define OBJ_{n.upper()} {events[n+"_event"]}')
lines+=['static const struct {uint16_t population,event,room_event,properties,enemy;uint8_t type;} objective_enemies[]={']
lines+=['{%d,%d,%d,%d,%d,%d},'%(r['population'],r['event'],r['roomEvent'],r['properties'],r['enemy'],r['type']) for r in rows]
lines+=['};','static const uint16_t objective_enemy_totals[6]={'+','.join(str(sum(r['type']==i for r in rows)) for i in range(6))+'};','static const uint16_t objective_enemy_all_events[6]={'+','.join(str(constants[k+'_all_event']) for k in ['space_pirates','ki_hunters','beetoms','cacatacs','kagos','yapping_maws'])+'};']
lines+=['static const struct {uint16_t enemy;uint32_t target;uint8_t kind;} objective_special_ai[]={']
lines+=['{%d,%d,%d},'%(r['enemy'],r['target'],['fish','geemer','shaktool','cacatac'].index(r['kind'])) for r in ai]
lines+=['};']
(root/'Native/sm_objective_events.inc').write_text('\n'.join(lines)+'\n')
print('OBJECTIVE_EVENTS',len(rows),'unique enemies',size,'event bytes',len(goals),'goal definitions',[(r['kind'],hex(r['enemy']),hex(r['target'])) for r in ai])
