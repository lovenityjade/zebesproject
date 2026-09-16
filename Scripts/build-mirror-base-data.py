#!/usr/bin/env python3
"""Compile reviewed Mirror data domains, recording unresolved code separately."""
import bisect,hashlib,json,sys
from pathlib import Path
root=Path(__file__).resolve().parents[1];sys.path.insert(0,str(root/'Randomizer/upstream'))
from rom.ips import IPS_Patch
catalog=json.loads((root/'Randomizer/native_mirror_locations.json').read_text())
original=(root/'roms/Super Metroid (Japan, USA) (En,Ja).sfc').read_bytes();shadow=bytearray(original)
assert hashlib.sha256(original).hexdigest()==catalog['sourceRomSha256']
names=[]
for line in (root/'native-core/assets/names.txt').read_text().splitlines():
 try:address,name=line.split(maxsplit=1);names.append((int(address,16),name))
 except ValueError:pass
names.sort();keys=[a for a,n in names]
programs=json.loads((root/'Docs/Randomizer/FullOptions/Mirror/scrolls.json').read_text())['programs']
code_ranges=[(p['address'],p['address']+len(p['bytes'])) for p in programs]+[(0x8f9194,0x8f91bb),(0x8fb971,0x8fb981),(0x8fe1d8,0x8fe1e8)]
def source_address(pc):return ((pc//32768)|128)<<16|(pc&32767)|32768
def symbol(address):return names[bisect.bisect_right(keys,address)-1][1]
def data_address(address,name):
 bank=address>>16
 if any(a<=address<b for a,b in [(0x84b876,0x84b88a),(0xa7e824,0xa7e838),(0xa7e87c,0xa7e890),(0xa7e8f6,0xa7e8fe),(0xa7e908,0xa7e90a),(0xa7e90c,0xa7e90e)]):return True
 if bank in [0x83,0x8c,0x8e,0xa1] or 0x95<=bank<=0x9f or bank>=0xb5:return True
 if 0x94b308<=address<0x950000 or 0x80c4b5<=address<0x80cb00:return True
 if bank==0x8f:return name=='bank_8f.ips' and not any(a<=address<b for a,b in code_ranges)
 # Named authored data in mixed banks is installed; C literal copies of such
 # tables remain a separate code-port requirement, listed in the audit.
 return symbol(address).startswith(('kPauseMenuMapData','kMap','kPlmDrawCmds','kPlmInstrList','kEproj','kCeres','kDead','kRidley','kDraygon','kKraid','kCrocomire','kTorizo','kBotwoon','kMotherBrain'))
ported=json.loads((root/'Native/sm_mirror_ports.json').read_text())['ranges']
approved=set();pending=[];native=[]
for patch in catalog['patches']:
 path=root/'Randomizer/upstream/patches/mirror/ips'/patch['name']
 if not path.exists():path=root/'Randomizer/upstream/patches/common/ips'/patch['name']
 assert hashlib.sha256(path.read_bytes()).hexdigest()==patch['sha256']
 for pc,values in IPS_Patch.load(path).toDict().items():
  shadow[pc:pc+len(values)]=bytes(values)
  rejected=[];translated=[]
  for i in range(len(values)):
   address=source_address(pc+i)
   if data_address(address,patch['name']):approved.add(pc+i)
   elif any(p['address']<=address<p['address']+p['size'] for p in ported):translated.append(address)
   else:rejected.append(address)
  if translated:native.append(dict(patch=patch['name'],address=translated[0],bytes=len(translated),symbol=symbol(translated[0])))
  if rejected:pending.append(dict(patch=patch['name'],address=rejected[0],bytes=len(rejected),symbol=symbol(rejected[0])))
assert hashlib.sha256(shadow).hexdigest()==catalog['patchedImageSha256']
# Item writes can touch bytes unchanged by Mirror; capture both complete entries.
item_bytes={a+d for r in catalog['locations'] for a in (r['canonicalAddress'],r['physicalAddress']) for d in range(6)}
changed=sorted({a for a in approved if shadow[a]!=original[a]} | item_bytes);spans=[];payload=[]
for address in changed:
 if not spans or spans[-1]['address']+spans[-1]['size']!=address:spans.append(dict(address=address,offset=len(payload),size=0))
 spans[-1]['size']+=1;payload.append(shadow[address])
lines=['/* Generated reviewed Mirror data only; unresolved routines remain guarded. */',
 '#define MIRROR_DATA_SIZE %d'%len(payload),
 'static const uint8_t mirror_bytes[]={'+','.join(map(str,payload))+'};',
 'static const struct {uint32_t address,offset,size;} mirror_spans[]={'+','.join('{%d,%d,%d}'%(s['address'],s['offset'],s['size']) for s in spans)+'};',
 'static const uint32_t mirror_item_addresses[100]={'+','.join(str(r['physicalAddress']) for r in catalog['locations'])+'};']
(root/'Native/sm_mirror_data.inc').write_text('\n'.join(lines)+'\n')
out=root/'Docs/Randomizer/FullOptions/Mirror'
(out/'data-audit.json').write_text(json.dumps(dict(spans=spans,payloadSha256=hashlib.sha256(bytes(payload)).hexdigest(),dataBytes=len(payload),pendingCode=pending,nativePortedCode=native,
 warning='Not full native Mirror support: named data may also have literal C copies, and all pending routines still need native adaptations.'),indent=2)+'\n')
print('MIRROR_DATA_CAPTURE',len(payload),'data bytes;',len(pending),'code records pending')
