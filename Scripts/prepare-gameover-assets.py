#!/usr/bin/env python3
"""Install the supplied Game Over art unchanged; decode its music for native playback."""
from pathlib import Path
import hashlib
import json
import shutil
import subprocess
import tempfile

root=Path(__file__).resolve().parents[1]
out=root/'Unreal/Content/GameOver';out.mkdir(parents=True,exist_ok=True)
manifest={'source':'User-supplied Game Over artwork and music','files':[]}
for source,name in [('gameover-16:9.png','GameOver-16x9.png'),('gameover-4:3.png','GameOver-4x3.png')]:
 shutil.copy2(root/source,out/name)
 digest=hashlib.sha256((root/source).read_bytes()).hexdigest()
 assert hashlib.sha256((out/name).read_bytes()).hexdigest()==digest
 manifest['files'].append(dict(source=source,runtime=name,sha256=digest))
with tempfile.TemporaryDirectory(prefix='sm-gameover-') as tmp:
 raw=Path(tmp)/'music.raw'
 subprocess.run(['ffmpeg','-v','error','-i',str(root/'gameover.mp3'),'-vn','-ar','44100','-ac','2','-f','s16le',str(raw)],check=True)
 data=raw.read_bytes();assert data and len(data)%4==0
 (out/'GameOver.pcm').write_bytes(b'MSU1'+bytes(4)+data)
 manifest['files'].append(dict(source='gameover.mp3',runtime='GameOver.pcm',sourceSha256=hashlib.sha256((root/'gameover.mp3').read_bytes()).hexdigest(),sha256=hashlib.sha256((out/'GameOver.pcm').read_bytes()).hexdigest(),frames=len(data)//4,sampleRate=44100,channels=2,loopFrame=0))
(out/'sources.json').write_text(json.dumps(manifest,indent=2)+'\n')
print('Installed Game Over art unchanged and decoded gameover.mp3.')
