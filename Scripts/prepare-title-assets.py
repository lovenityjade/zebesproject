#!/usr/bin/env python3
"""Stage the reviewed title layers unchanged; never bundle the extracted font."""
import hashlib
import json
import os
from pathlib import Path
import shutil
import sys
from PIL import Image

assert os.uname().nodename == 'gaming-pc', 'Prepare artwork on gaming-pc only'
root = Path(sys.argv[1]).resolve() if len(sys.argv) > 1 else Path(__file__).resolve().parents[1]
out = root / 'Unreal/Content/Title'; out.mkdir(parents=True, exist_ok=True)
lab = json.loads((root/'Labs/TitleScreen/manifest.json').read_text())
entries = {}
for key, info in lab.items():
    if key == 'font': continue  # decoded from the user's verified ROM at runtime
    src = (root/'Labs/TitleScreen'/info['src']).resolve()
    digest = hashlib.sha256(src.read_bytes()).hexdigest()
    assert digest == info['sha256'], f'Reviewed layer changed: {key}'
    shutil.copy2(src, out/f'{key}.png')
    entries[key] = {'file':f'{key}.png', 'size':info['size'], 'crop':info['crop'], 'sha256':digest}
src = root/'thelovenityjade-logo.png'
im = Image.open(src).convert('RGBA')
box = im.getchannel('A').point(lambda a:255 if a>=45 else 0).getbbox()
shutil.copy2(src, out/'creator.png')
entries['creator'] = {'file':'creator.png','size':im.size,'crop':box,
                      'sha256':hashlib.sha256(src.read_bytes()).hexdigest()}
(out/'layers.json').write_text(json.dumps(entries,indent=2)+'\n')
print(f'Staged {len(entries)} original reviewed layers; font excluded.')
