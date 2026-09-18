"""Build the deliberately small static site for the existing Caddy VPS.

Copies only public/, verifies all local HTML/CSS references, and writes a hash
manifest outside the public tree. No ROM, save, credentials or server code.
"""
import hashlib
import json
import re
import shutil
from html.parser import HTMLParser
from pathlib import Path

ROOT=Path(__file__).resolve().parent
SOURCE=ROOT/'public'
DEST=ROOT/'dist'


class References(HTMLParser):
    def __init__(self):super().__init__();self.paths=[];self.ids=set();self.hashes=[]
    def handle_starttag(self,tag,attrs):
        attrs=dict(attrs)
        if 'id' in attrs:
            assert attrs['id'] not in self.ids, 'Duplicate id '+attrs['id']
            self.ids.add(attrs['id'])
        for key in ('href','src'):
            value=attrs.get(key,'')
            if value.startswith('/zebes/') and not value.startswith('/zebes/api/'):
                self.paths.append(value.split('?')[0].split('#')[0].removeprefix('/zebes/'))
            if value.startswith('#'):self.hashes.append(value[1:])


from localize import generate
generate()

for page in SOURCE.rglob('*.html'):
    parser=References();parser.feed(page.read_text())
    for path in parser.paths:
        assert (SOURCE/path).exists(),(page,path)
    for fragment in parser.hashes:assert fragment in parser.ids,(page,fragment)
for stylesheet in SOURCE.rglob('*.css'):
    for path in re.findall(r"url\(['\"]?(/zebes/[^'\")]+)",stylesheet.read_text()):
        assert (SOURCE/path.removeprefix('/zebes/')).is_file(),path
files=sorted(path for path in SOURCE.rglob('*') if path.is_file())
allowed={'.html','.css','.js','.png','.webp','.ttf','.txt','.xml'}
for path in files:
    assert path.suffix in allowed and not path.name.startswith('.'),path
if DEST.exists():shutil.rmtree(DEST)
shutil.copytree(SOURCE,DEST)
report={'prefix':'/zebes/','files':{},'total_bytes':0}
for path in files:
    data=path.read_bytes();report['files'][str(path.relative_to(SOURCE))]=hashlib.sha256(data).hexdigest();report['total_bytes']+=len(data)
(ROOT/'build-manifest.json').write_text(json.dumps(report,indent=2)+'\n')
print(f'Built {len(files)} public files, {report["total_bytes"]:,} bytes; local references validated.')
