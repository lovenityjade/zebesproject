"""Lossless format conversion only. Original source pixels remain unchanged."""
from pathlib import Path
import hashlib
import json
from PIL import Image

root=Path(__file__).resolve().parent
assets=root/'public/assets'
manifest=json.loads((root/'asset-provenance.json').read_text())
saved=0
for entry in manifest:
    source=assets/entry['file']
    if source.suffix!='.png' or source.name in ('tablet.png','title.png'):
        continue
    target=source.with_suffix('.webp')
    image=Image.open(source).convert('RGBA')
    image.save(target,'WEBP',lossless=True,exact=True,method=4)
    assert Image.open(target).convert('RGBA').tobytes()==image.tobytes(),source.name
    saved+=source.stat().st_size-target.stat().st_size
    entry.update(file=target.name,source_sha256=entry['sha256'],sha256=hashlib.sha256(target.read_bytes()).hexdigest(),treatment='Lossless WebP container conversion; identical decoded RGBA pixels verified')
    for path in [root/'public/index.html',root/'public/site.css']:
        path.write_text(path.read_text().replace('/assets/'+source.name,'/assets/'+target.name))
    source.unlink()
(root/'asset-provenance.json').write_text(json.dumps(manifest,indent=2)+'\n')
print(f'Lossless assets: saved {saved:,} bytes; exact decoded pixels verified.')
