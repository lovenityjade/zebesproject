#!/usr/bin/env python3
"""Build pre-ROM application icons from the project-owned logo only.
The Samus helmet is decoded from the player ROM after validation at runtime.
"""
from pathlib import Path
import struct
from PIL import Image

root = Path(__file__).resolve().parents[1]
source = root / 'zebesproject-logo.png'
output = root / 'Unreal/Content/Splash'
output.mkdir(parents=True, exist_ok=True)
with Image.open(source) as original:
    art = original.convert('RGBA')
    art.thumbnail((64,64),Image.Resampling.LANCZOS)
    icon = Image.new('RGBA',(64,64))
    icon.alpha_composite(art,((64-art.width)//2,(64-art.height)//2))
pixels = icon.tobytes('raw', 'BGRA')
# BITMAPV4HEADER: explicit RGBA masks avoid losing alpha in SDL_LoadBMP.
header = struct.pack('<IiiHHIIiiII', 108, 64, -64, 1, 32, 3, len(pixels), 2835, 2835, 0, 0)
header += struct.pack('<IIIII', 0xff0000, 0xff00, 0xff, 0xff000000, 0x73524742) + bytes(48)
data = struct.pack('<2sIHHI', b'BM', 122 + len(pixels), 0, 0, 122) + header + pixels
for name in ('Icon.bmp', 'EdIcon.bmp'):
    (output / name).write_bytes(data)

# Windows executable/taskbar placeholder before the user supplies a ROM.
windows=root/'Unreal/Build/Windows';windows.mkdir(parents=True,exist_ok=True)
icon.save(windows/'Application.ico',sizes=[(16,16),(32,32),(48,48),(64,64)])
import hashlib,json
proof=dict(source='zebesproject-logo.png',sourceSha256=hashlib.sha256(source.read_bytes()).hexdigest(),
           files={str(p.relative_to(root)):hashlib.sha256(p.read_bytes()).hexdigest()
                  for p in [output/'Icon.bmp',output/'EdIcon.bmp',windows/'Application.ico']},
           policy='Project-owned startup logo; original helmet decoded from verified ROM at runtime')
(output/'provenance.json').write_text(json.dumps(proof,indent=2)+'\n')
