#!/usr/bin/env python3
"""Compile area routing and reviewed dependencies from the pinned VARIA source.

This is a source compiler, not a ROM/game test. CPU patch bytes are recorded for
audit and excluded from the native data image. Boss IDs remain independent.
"""
import hashlib
import json
import re
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
upstream = root / 'Randomizer/upstream'
sys.path.insert(0, str(upstream))
from logic.logic import Logic
Logic.factory('vanilla')
from graph.graph import AccessGraph
from graph.graph_utils import GraphUtils, vanillaTransitions
from rom.flavor import RomFlavor
from rom.ips import IPS_Patch
from rom.rom_patches import getPatchSet
RomFlavor.factory(str(upstream))
access = RomFlavor.patchAccess

graph = AccessGraph(Logic.accessPoints(), vanillaTransitions)
GraphUtils.getDoorConnections(graph, True, False, False)
names = sorted({n for pair in vanillaTransitions for n in pair})
assert len(names) == 32
aps = [graph.accessPoints[n] for n in names]
native_source = (root/'native-core/src/sm_8f.c').read_text()
dispatch = native_source[native_source.index('void CallDoorDefSetupCode(uint32 ea) {'):]
records = []
for i, ap in enumerate(aps):
    asm = ap.EntryInfo['doorAsmPtr']
    if asm:
        match = re.search(r'void (\w+)\(void\) \{  // 0x8F%04X' % asm, native_source)
        assert match and 'case fn'+match[1]+':' in dispatch, (ap.Name, asm)
    records.append(dict(id=i, name=ap.Name, graphArea=ap.GraphArea,
                        room=ap.RoomInfo, entry=ap.EntryInfo, exit=ap.ExitInfo))

matrix = []
for i, src in enumerate(aps):
    row = []
    for j, dst in enumerate(aps):
        pair = AccessGraph(Logic.accessPoints(), [(src.Name, dst.Name)])
        actual = GraphUtils.getDoorConnections(pair, True, False, False)
        c, = [c for c in actual if c['transition'][0].Name == src.Name]
        assert c['transition'][1].Name == dst.Name
        assert c.get('exitAsm') in (None, 'door_transition_boss_exit_fix')
        assert isinstance(c['doorAsmPtr'], int)
        row.append(dict(source=i, destination=j, room=c['RoomPtr'], door=c['DoorPtr'],
                        direction=c['direction'], bitFlag=c['bitFlag'], cap=c['cap'],
                        screen=c['screen'], asm=c['doorAsmPtr'], exitAsm=c.get('exitAsm'),
                        distance=c['distanceToSpawn'], incompatible='SamusX' in c,
                        x=dst.EntryInfo['SamusX'], y=dst.EntryInfo['SamusY']))
    matrix.append(row)

# VARIA applies area dependencies before layout/tweaks and applies blinking
# replacements after those groups. Native ordering preserves these two phases.
area = getPatchSet('area', 'vanilla')
base_names = area['ips']
blink_names = ['Blinking['+ap.Name+']' for ap in Logic.accessPoints()
               if not ap.Internal and not ap.Boss]
blink_names += ['Blinking[West Sand Hall Left]', 'Blinking[Below Botwoon Energy Tank Right]']
dict_patches = access.getDictPatches()
blink_names = list(dict.fromkeys(n for n in blink_names if n in dict_patches))
plm_names = list(area.get('plms', [])) + ['Maridia Sand Hall Seal', 'Save_Main_Street', 'Save_Crab_Shaft']
plm_names += ['Blinking['+ap.Name+']' for ap in Logic.accessPoints()
              if not ap.Internal and not ap.Boss]
plm_names += ['Blinking[West Sand Hall Left]', 'Blinking[Below Botwoon Energy Tank Right]']
plms = {n:access.getAdditionalPLMs()[n] for n in dict.fromkeys(plm_names)
        if n in access.getAdditionalPLMs()}

data = []
spans = []
patches = []
excluded = []
for phase, patch_names in enumerate([base_names, blink_names]):
    for name in patch_names:
        if name in dict_patches:
            raw = dict_patches[name]
            sha = hashlib.sha256(json.dumps(raw, sort_keys=True).encode()).hexdigest()
        else:
            p = Path(access.getPatchPath(name))
            raw = IPS_Patch.load(str(p)).toDict()
            sha = hashlib.sha256(p.read_bytes()).hexdigest()
        patch = dict(name=name, phase=phase, sha256=sha, spans=[])
        for address, values in sorted(raw.items()):
            run = []
            start = address
            for n, value in enumerate(values):
                a = address+n
                cpu = name == 'door_transition.ips' or (name == 'area_rando_doors.ips' and 0x7f701 <= a < 0x7f720)
                if cpu:
                    if run:
                        span = dict(address=start, offset=len(data), size=len(run), phase=phase, patch=name)
                        spans.append(span);patch['spans'].append(dict(address=start, data=run));data.extend(run);run=[]
                    excluded.append(dict(patch=name, address=a, value=value))
                else:
                    if not run:start=a
                    run.append(value)
            if run:
                spans.append(dict(address=start, offset=len(data), size=len(run), phase=phase, patch=name))
                patch['spans'].append(dict(address=start, data=run));data.extend(run)
        patches.append(patch)
assert sum(x['patch']=='area_rando_doors.ips' for x in excluded)==31
assert all(set(p) <= {'room','state','plm_bytes_list'} for p in plms.values())
assert all(len(b)==6 for p in plms.values() for b in p['plm_bytes_list'])

catalog = dict(schema=1, upstream='72ec1f30b700442d0c30aad6c43801bd64f1e2a5',
               accessPoints=records, connections=matrix, vanillaPairs=vanillaTransitions,
               patches=patches, roomPlms=plms, spans=spans,
               dataSha256=hashlib.sha256(bytes(data)).hexdigest(),
               nativeCode=['door_transition.ips', 'area_rando_doors:full_refill'],
               westOceanSky=0x7b7bb)
catalog['sha256'] = hashlib.sha256(json.dumps(catalog,sort_keys=True,separators=(',',':')).encode()).hexdigest()
for path in (root/'Randomizer/area_catalog_history').glob('*.json'):
    assert json.loads(path.read_text())==json.loads(json.dumps(catalog)), 'Published area catalog changed: '+path.name

from world_rom_assets import build as build_rom_recipe
recipe_ranges=[];recipe_meta=[]
for patch in patches:
    indices=[i for i,s in enumerate(spans) if s['patch']==patch['name'] and s['phase']==patch['phase']]
    first=indices[0] if indices else 0
    assert indices==list(range(first,first+len(indices)))
    recipe_meta.append(dict(id=len(recipe_meta),name=patch['name']))
    recipe_ranges.append((first,len(indices)))
build_rom_recipe(root,recipe_meta,recipe_ranges,[(s['address'],s['offset'],s['size']) for s in spans],data,{},domain='areas')
# Preserve the catalog identity used by existing seeds; it identifies the full
# pinned behavior/payload, not the JSON serialization with payloads omitted.
for patch in patches:
    for span in patch['spans']:
        payload=bytes(span.pop('data'))
        span.update(size=len(payload),sha256=hashlib.sha256(payload).hexdigest())
lines = ['/* Generated area routing and data; no injected CPU instructions. */',
         '#define AREA_AP_COUNT 32', f'#define AREA_DATA_SIZE {len(data)}',
         'static const char area_catalog[]="'+catalog['sha256']+'";',
         f'static uint8_t area_data[{len(data)}];',
         'static const struct {uint32_t address,offset,size;uint8_t phase;} area_spans[]={']
lines += ['{%d,%d,%d,%d},'%(s['address'],s['offset'],s['size'],s['phase']) for s in spans]
lines += ['};','static const struct {uint16_t door,room;uint8_t song_count,song;uint16_t songs[3];} area_aps[]={']
for ap in aps:
    songs=ap.RoomInfo.get('songs',[]);assert len(songs)<=3
    lines.append('{%d,%d,%d,%d,{%s}},'%(ap.ExitInfo['DoorPtr'],ap.RoomInfo['RoomPtr'],len(songs),ap.EntryInfo.get('song',0),','.join(map(str,songs+[0]*(3-len(songs))))))
lines += ['};','typedef struct {uint8_t direction,flag,cap_x,cap_y,screen_x,screen_y,incompatible,exit_fix;uint16_t distance,asm_ptr,x,y;} AreaConnection;',
          'static const AreaConnection area_matrix[32][32]={']
for row in matrix:
    lines.append('{')
    for c in row:
        lines.append('{%s},'%','.join(map(str,[c['direction'],c['bitFlag'],*c['cap'],*c['screen'],int(c['incompatible']),2 if c['exitAsm'] else 0,c['distance'],c['asm'],c['x'],c['y']])))
    lines.append('},')
lines += ['};','static const struct {uint16_t room,state;uint8_t entry[6];} area_plms[]={']
for p in plms.values():
    for b in p['plm_bytes_list']:
        lines.append('{%d,%d,{%s}},'%(p['room'],p.get('state',0),','.join(map(str,b))))
lines.append('};')
(root/'Native/sm_areas.inc').write_text('\n'.join(lines)+'\n')
(root/'Randomizer/native_areas.json').write_text(json.dumps(catalog,indent=2)+'\n')
(root/'Unreal/Source/SMUnreal/SMNativeAreaNames.inl').write_text('// Generated immutable area endpoint order.\nstatic const TArray<FString> NativeAreaNames={\n'+''.join('TEXT('+json.dumps(n)+'),\n' for n in names)+'};\n')
out=root/'Docs/Randomizer/FullOptions/Areas';out.mkdir(parents=True,exist_ok=True)
(out/'audit.json').write_text(json.dumps(dict(catalogSha256=catalog['sha256'],excluded=excluded,
    originalAsm={a.Name:a.EntryInfo['doorAsmPtr'] for a in aps},patches=patches),indent=2)+'\n')
print('AREA_CATALOG',len(aps),'APs',sum(map(len,matrix)),'directed pairs',len(data),'data bytes',len(plms),'PLM templates')
