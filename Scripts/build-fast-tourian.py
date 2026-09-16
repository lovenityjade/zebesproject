#!/usr/bin/env python3
"""Extract only authored room data; executable source is ported in C overlays."""
import hashlib,json
from pathlib import Path
root=Path(__file__).resolve().parents[1]
path=root/'Randomizer/upstream/patches/vanilla/ips/open_zebetites.ips'
b=path.read_bytes();assert b[:5]==b'PATCH';i=5;spans=[]
while b[i:i+3]!=b'EOF':
 a=int.from_bytes(b[i:i+3],'big');n=int.from_bytes(b[i+3:i+5],'big');i+=5
 if n:d=b[i:i+n];i+=n
 else:n=int.from_bytes(b[i:i+2],'big');d=b[i+2:i+3]*n;i+=3
 assert 0x26df17<=a and a+n<=0x26e20e
 spans.append((a,d))
assert [(a,len(d)) for a,d in spans]==[(0x26df17,1),(0x26df22,448),(0x26e0e2,300)]
spans+=[(0x7a616,bytes.fromhex('5c aa')),(0x1aa5e,b'\x40'),(0x7ddeb,bytes.fromhex('16 92')),(0x19218,b'\x40')]
lines=['/* Authored Fast Tourian room/door data. No executable patch bytes. */',
 'static const struct {uint32_t address;uint16_t size;uint8_t data[448];} tourian_data[]={']
lines+=['{%d,%d,{%s}},'%(a,len(d),','.join(map(str,d))) for a,d in spans]
lines+=['};','#define TOURIAN_DATA_BYTES '+str(sum(len(d) for a,d in spans))]
(root/'Native/sm_tourian_data.inc').write_text('\n'.join(lines)+'\n')
out=root/'Docs/Randomizer/FullOptions/FastTourian';out.mkdir(exist_ok=True)
(out/'data-audit.json').write_text(json.dumps(dict(spans=[dict(address=a,size=len(d),sha256=hashlib.sha256(d).hexdigest()) for a,d in spans],sourceSha256=hashlib.sha256(b).hexdigest(),scope='Room and door data; native code follows minimizer_tourian_common.asm'),indent=2)+'\n')
