#!/usr/bin/env python3
"""Compile reviewed VARIA graphics/room metadata; no generated artwork or ROM writes."""
import ast,json,re,hashlib
from ui_rom_assets import recipe
from pathlib import Path
root=Path(__file__).resolve().parents[1];src=root/'Randomizer/upstream/patches/common/src'
data=bytearray((src/'map/hud.gfx').read_bytes());assert len(data)==4096
address=0
for line in (src/'max_ammo_display.asm').read_text().splitlines():
 line=line.split(';')[0].strip()
 if m:=re.match(r'org \$([0-9A-Fa-f]+)',line):address=int(m[1],16)
 if line.startswith('db ') and 0x9ab200<=address<0x9ac200:
  values=bytes(int(x,16) for x in re.findall(r'\$([0-9a-fA-F]{2})',line));offset=address-0x9ab200
  data[offset:offset+len(values)]=values;address+=len(values)
regions=['Ceres','Crateria','GreenPinkBrinstar','RedBrinstar','WreckedShip','Kraid','Norfair','Crocomire','LowerNorfair','WestMaridia','EastMaridia','Tourian']
tree=ast.parse((root/'Randomizer/upstream/tools/rooms.py').read_text());rooms=next(ast.literal_eval(n.value) for n in tree.body if isinstance(n,ast.Assign) and any(isinstance(t,ast.Name) and t.id=='rooms' for t in n.targets))
byroom={r['Address']&65535:regions.index(r['GraphArea']) for r in rooms}
locations=json.loads((root/'Randomizer/native_locations.json').read_text())['locations']
font={line[0]:int(line[2:],16) for line in (src/'tables/hud_chars.txt').read_text().splitlines() if len(line)>2 and line[1]=='='}
out=['/* VARIA HUD lettering/widgets; original tiles come from the player ROM. */']
out += recipe(root,'varia_gfx',data,2,0x9ab200,8192,set(range(256)))
out += ['static const uint16_t varia_font[128] = {',','.join(str(font.get(chr(c),0xf)) for c in range(128)),'};']
# Current room region comes from the layout-specific map exploration catalog.
out+=['static const uint8_t varia_locations[100][4] = {']
out += ['{%d,%d,%d,%d},'%(byroom[c['roomPointer']],c['area'],c['mapX'],c['mapY']) for c in sorted(locations,key=lambda c:c['address'])];out+=['};']
(root/'Native/sm_varia_assets.inc').write_text('\n'.join(out)+'\n')
