#!/usr/bin/env python3
"""Install the approved local audition selection. No downloads or ROM changes."""
import argparse, hashlib, json, shutil, tempfile
from pathlib import Path
root=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--source',type=Path,default=root/'.tmp/msu-audition')
p.add_argument('--destination',type=Path,default=root/'Soundtracks/Remastered')
a=p.parse_args();manifest=json.loads((root/'Config/Soundtracks/remastered.json').read_text())
# Validate the full source set before copying anything.
for t in manifest['tracks']:
 f=a.source/t['source_path']
 with f.open('rb') as stream: actual=hashlib.file_digest(stream,'sha256').hexdigest()
 if actual!=t['sha256']:raise SystemExit(f'Hash mismatch: {f}')
a.destination.mkdir(parents=True,exist_ok=True)
for t in manifest['tracks']:
 dest=a.destination/t['file']
 if dest.exists():
  with dest.open('rb') as stream: actual=hashlib.file_digest(stream,'sha256').hexdigest()
  if actual==t['sha256']:continue
  raise SystemExit(f'Existing different file preserved: {dest}')
 with tempfile.NamedTemporaryFile(dir=a.destination,prefix='.install-',delete=False) as temp:
  tmp=Path(temp.name)
 try:
  shutil.copyfile(a.source/t['source_path'],tmp)
  with tmp.open('rb') as stream: assert hashlib.file_digest(stream,'sha256').hexdigest()==t['sha256']
  tmp.replace(dest)
 finally:tmp.unlink(missing_ok=True)
(a.destination/'manifest.json').write_text(json.dumps(manifest,indent=2,ensure_ascii=False)+'\n')
shutil.copy2(root/'Docs/REMASTERED-SOUNDTRACK.md',a.destination/'CREDITS.md')
print(f'Installed and verified {len(manifest["tracks"])} tracks: {a.destination}')
