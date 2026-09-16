#!/usr/bin/env python3
"""Read Mirror's actual patched room/item data without executing patched code."""
import hashlib,json,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];upstream=root/'Randomizer/upstream'
sys.path.insert(0,str(upstream))
from logic.logic import Logic
from rom.flavor import RomFlavor
from rom.rom_patches import getPatchSet
from rom.ips import IPS_Patch
Logic.factory('mirror');RomFlavor.factory(str(upstream))
rom=bytearray((root/'roms/Super Metroid (Japan, USA) (En,Ja).sfc').read_bytes())
source_hash=hashlib.sha256(rom).hexdigest()
assert source_hash=='12b77c4bc9c1832cee8881244659065ee1d84c70c3d29e6eaf92e6798cc2ca72'
patches=[]
for name in getPatchSet('logic','mirror')['ips']:
 path=Path(RomFlavor.patchAccess.getPatchPath(name));spans=IPS_Patch.load(path).toDict()
 for address,data in spans.items():rom[address:address+len(data)]=bytes(data)
 patches.append(dict(name=name,sha256=hashlib.sha256(path.read_bytes()).hexdigest(),spans=[dict(address=a,size=len(d)) for a,d in spans.items()]))
canonical=json.loads((root/'Randomizer/native_locations.json').read_text())['locations']
locations={loc.Name:loc for loc in Logic.locations() if not loc.isBoss()}
rows=[]
for index,original in enumerate(canonical):
 loc=locations[original['name']];address=loc.Address;pc=original['roomHeaderPc']
 plm=int.from_bytes(rom[address:address+2],'little');bit=int.from_bytes(rom[address+4:address+6],'little')&255
 assert 0xeed7<=plm<=0xefcf and (plm-0xeed7)%4==0,(loc.Name,hex(address),hex(plm))
 assert bit==loc.Id==original['collectionBit'],loc.Name
 x,y=rom[address+2:address+4];width,height=rom[pc+4:pc+6]
 assert x<width*16 and y<height*16,loc.Name
 rows.append(dict(index=index,name=loc.Name,canonicalAddress=original['address'],physicalAddress=address,collectionBit=bit,
  area=rom[pc+1],roomPointer=original['roomPointer'],roomMapX=rom[pc+2],roomMapY=rom[pc+3],roomWidth=width,roomHeight=height,
  plmX=x,plmY=y,mapX=rom[pc+2]+(x>>4),mapY=rom[pc+3]+(y>>4)+1,
  sourceMapX=loc.MapAttrs.X,sourceMapY=loc.MapAttrs.Y,visibility=loc.Visibility))
assert len(rows)==100 and len({r['physicalAddress'] for r in rows})==100
relocated=[r['name'] for r in rows if r['canonicalAddress']!=r['physicalAddress']]
assert set(relocated)=={'Reserve Tank, Wrecked Ship','Missile (Gravity Suit)'}
data=dict(schema=1,sourceRomSha256=source_hash,patchedImageSha256=hashlib.sha256(rom).hexdigest(),patches=patches,locations=rows,
 scope='Base Mirror logic patch sequence; source coordinates and identities only. Not an enabled native world or seed.')
(root/'Randomizer/native_mirror_locations.json').write_text(json.dumps(data,indent=2)+'\n')
print('MIRROR_GEOMETRY_PASS',len(rows),'checks, relocated',relocated)
