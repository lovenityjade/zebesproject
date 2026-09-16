#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
mkdir -p Docs/Depth
Scripts/play.sh -windowed -ResX=1280 -ResY=720 -SMDepthMotion -SMTestSavedRoom -SMWarmup=8500 -unattended > .tmp/unreal-depth-motion.log 2>&1
python3 - <<'PY'
from pathlib import Path
from PIL import Image
import re
import numpy as np
log=Path('.tmp/unreal-depth-motion.log').read_text(errors='replace')
motion=re.findall(r'SM_DEPTH_CAMERA player=(\d+,\d+) camera=(\d+,\d+)',log)
assert len({p for p,c in motion})>1 and len({c for p,c in motion})>1,'No player/camera movement'
assert 'SM_DEPTH_MOTION_DONE frames=60 room=91f8 cpu_opcodes=0' in log
files=[Path('Unreal/Saved/SMTests/depth-motion')/f'frame-{i:04d}.png' for i in range(60)]
assert all(p.exists() for p in files)
a=np.array(Image.open(files[0]));b=np.array(Image.open(files[-1]))
assert np.any(a!=b),'Motion capture stayed frozen'
print('60 actual Unreal frames captured with native camera/player motion; 0 emulated CPU opcodes.')
PY
ffmpeg -hide_banner -loglevel error -y -framerate 15 -i Unreal/Saved/SMTests/depth-motion/frame-%04d.png -frames:v 60 -c:v libx264 -crf 18 -pix_fmt yuv420p Docs/Depth/Crateria-motion.mp4
