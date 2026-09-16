#!/usr/bin/env python3
"""One-time conversion of frozen area/escape/animals data into ROM recipes.
The ordinary domain generators reproduce these recipes from their pinned upstream.
Requires original initialized includes for this migration; do not stage fixtures.
"""
from pathlib import Path
import re,json
from world_rom_assets import build
root=Path(__file__).resolve().parents[1]
def array(path,name):
 s=path.read_text();m=re.search(r'\b'+name+r'\[\]\s*=\s*\{(.*?)\};',s,re.S);assert m,name
 return m,[int(v,0) for v in re.findall(r'0x[0-9a-fA-F]+|\d+',m[1])]
for domain,filename,payload_name,span_name,range_name in [
 ('areas','sm_areas.inc','area_data','area_spans',None),
 ('escape','sm_escape_data.inc','escape_room_data','escape_room_spans',None),
 ('animals','sm_animals_data.inc','animals_bytes','animals_spans','animals_variants')]:
 path=root/'Native'/filename;m,data=array(path,payload_name);_,flat=array(path,span_name)
 if domain=='areas':
  catalog=json.loads((root/'Randomizer/native_areas.json').read_text());spans=[tuple(flat[i:i+3]) for i in range(0,len(flat),4)]
  metadata=[];ranges=[]
  for patch in catalog['patches']:
   indices=[i for i,s in enumerate(catalog['spans']) if s['patch']==patch['name'] and s['phase']==patch['phase']]
   first=indices[0] if indices else 0;assert indices==list(range(first,first+len(indices)))
   metadata.append(dict(id=len(metadata),name=patch['name']));ranges.append((first,len(indices)))
 elif domain=='escape':
  spans=[(flat[i],flat[i+2],flat[i+1]) for i in range(0,len(flat),3)];metadata=[dict(id=0,name='Escape room changes')];ranges=[(0,len(spans))]
 else:
  spans=[tuple(flat[i:i+3]) for i in range(0,len(flat),3)];_,v=array(path,range_name)
  ranges=[tuple(v[i:i+2]) for i in range(0,len(v),2)];metadata=[dict(id=i,name='Animal surprise '+str(i)) for i in range(len(ranges))]
 build(root,metadata,ranges,spans,data,{},domain=domain)
 text=path.read_text();start=text.index('static const uint8_t '+payload_name)
 end=text.index(';',start)+1
 path.write_text(text[:start]+f'static uint8_t {payload_name}[{len(data)}];'+text[end:])
 print(domain,len(data),'bytes reconstructed')
