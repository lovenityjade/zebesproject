#!/usr/bin/env python3
"""Inventory every named graphics set and enemy definition from the verified ROM.
This is an extraction/review catalogue, not an automatic declaration of emitters.
"""
from pathlib import Path
import struct,json,hashlib,re
import numpy as np
from PIL import Image,ImageDraw
root=Path(__file__).resolve().parents[1]; out=root/'Docs/LightingAudit/Inventory';out.mkdir(parents=True,exist_ok=True)
rom=(root/'roms/Super Metroid (Japan, USA) (En,Ja).sfc').read_bytes()
assert hashlib.sha1(rom).hexdigest()=='da957f0d63d14cb441d215462904c4fa8519c613'
def off(addr): return ((addr>>16)&127)*32768+(addr&32767)
def u16(a): return struct.unpack_from('<H',rom,off(a))[0]
def u24(a): return int.from_bytes(rom[off(a):off(a)+3],'little')
def unpack(a):
    pos=off(a);buf=bytearray()
    def take():
        nonlocal pos
        x=rom[pos];pos+=1;return x
    while True:
        b=take()
        if b==255:return bytes(buf)
        cmd=b>>5;n=(b&31)+1
        if cmd==7: cmd=(b>>2)&7;n=((b&3)<<8|take())+1
        if cmd==0:buf.extend(take() for _ in range(n))
        elif cmd==1:buf.extend(bytes([take()])*n)
        elif cmd==2:
            a,b=take(),take();buf.extend((a,b)[i%2] for i in range(n))
        elif cmd==3:
            a=take();buf.extend((a+i)&255 for i in range(n))
        else:
            src=len(buf)-take() if cmd>=6 else take()|(take()<<8)
            mask=255 if cmd&1 else 0
            for i in range(n):buf.append(buf[src+i]^mask)
        if len(buf)>262144:raise ValueError('Invalid compressed resource')
def palette(data):
    p=np.frombuffer(data[:32],'<u2').astype(np.uint32)
    return np.stack([((p>>k)&31)*255//31 for k in (0,5,10)],axis=1).astype(np.uint8)
def tilesheet(data,pal):
    n=len(data)//32
    image=Image.new('RGB',(128,max(8,((n+15)//16)*8)),(0,0,0))
    for i in range(n):
        b=data[i*32:(i+1)*32];idx=np.zeros((8,8),np.uint8)
        for y in range(8):
            for x in range(8):idx[y,x]=sum(((b[y*2+(p&1)+(p//2)*16]>>(7-x))&1)<<p for p in range(4))
        image.paste(Image.fromarray(pal[idx]),((i%16)*8,(i//16)*8))
    return image
records=[]
for i in range(29):
    addr=0x8f0000|u16(0x8fe7a7+i*2)
    table,gfx,pal=(u24(addr+j) for j in (0,3,6))
    data=unpack(gfx);p=unpack(pal)
    (out/f'tiletable-{i:02d}.bin').write_bytes(unpack(table))
    rec={'kind':'graphics_set','index':i,'definition':hex(addr),'graphics_pointer':hex(gfx),'palette_pointer':hex(pal),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'status':'needs-review','format_note':'Room renderer determines 4bpp vs Mode7; see runtime audits.'}
    (out/f'graphics-{i:02d}.bin').write_bytes(data)
    (out/f'palette-{i:02d}.bin').write_bytes(p)
    records.append(rec)
names=dict(line.split(' ',1) for line in (root/'native-core/assets/names.txt').read_text().splitlines() if ' ' in line)
enemies=[]
for line in (root/'native-core/assets/names.txt').read_text().splitlines():
    match=re.fullmatch(r'(0x[0-9a-fA-F]+) (kEnemyDef_\S+)',line)
    if not match:continue
    a=int(match[1],16);name=match[2].removeprefix('kEnemyDef_')
    n=u16(a)&32767;bank=rom[off(a)+12];pp=(bank<<16)|u16(a+2);gp=u24(a+54)
    data=rom[off(gp):off(gp)+n] if gp&32768 else b''
    pd=rom[off(pp):off(pp)+32]
    handler=names.get(hex((bank<<16)|u16(a+18)),'unknown')
    rec={'init_handler':handler,'kind':'enemy','name':name,'definition':hex(a),'bytes':n,'graphics_pointer':hex(gp),'palette_pointer':hex(pp),'status':'needs-review','sha256':hashlib.sha256(data).hexdigest()}
    if len(pd)==32 and data and n%32==0:
        sheet=tilesheet(data,palette(pd));sheet.resize((sheet.width*2,sheet.height*2),Image.Resampling.NEAREST).save(out/f'enemy-{a:06x}.png')
        rec['sheet']=f'enemy-{a:06x}.png';enemies.append((rec,sheet))
    records.append(rec)
for page in range((len(enemies)+19)//20):
    canvas=Image.new('RGB',(1024,5*164),(22,25,30));draw=ImageDraw.Draw(canvas)
    for j,(rec,sheet) in enumerate(enemies[page*20:page*20+20]):
        x=(j%4)*256;y=(j//4)*164
        thumb=sheet.crop((0,0,128,min(sheet.height,64))).resize((256,min(sheet.height,64)*2),Image.Resampling.NEAREST)
        canvas.paste(thumb,(x,y));draw.text((x+4,y+131),rec['name'][:33],fill='white');draw.text((x+4,y+147),rec['definition']+'  '+str(rec['bytes'])+' bytes',fill=(130,165,180))
    canvas.save(out/f'enemies-overview-{page:02d}.png')
raw_records=[]
for source in sorted((root/'supermetroid/graphics').glob('*.bin')):
    data=source.read_bytes()
    raw_records.append({'path':str(source.relative_to(root)),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest(),'status':'needs-format-review'})
manifest={'raw_graphics_files':raw_records,'rom_sha1':hashlib.sha1(rom).hexdigest(),'graphics_sets':29,'enemy_definitions':sum(r['kind']=='enemy' for r in records),'enemy_sheets':len(enemies),'records':records,'notice':'Inventory complete for named tables; manual emissive review is tracked separately. Not all runtime animations or palettes are validated.'}
(out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(json.dumps({k:v for k,v in manifest.items() if k!='records'},indent=2))
