#!/usr/bin/env python3
"""Isolated generated-connection arrival and original pause map under Vulkan."""
import hashlib,json,os,shutil,subprocess,sys,time
from pathlib import Path
root=Path(sys.argv[1]).resolve();assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
assert subprocess.run(['pgrep','-x','SMUnreal'],stdout=subprocess.DEVNULL).returncode==1
out=root/'map-exploration';lib=root/'Native/build/libsm_native.so'
shutil.copy2(out/'libsm_native.so',lib.with_suffix('.so.next'));os.replace(lib.with_suffix('.so.next'),lib)
args=['./SMUnreal/Binaries/Linux/SMUnreal','/Engine/Maps/Entry','-SMAreaTest','-SMStartTest=0','-SMAreaConnectionTest=24','-SMPauseTest','-SMWarmup=12000','-RenderOffscreen','-unattended','-windowed','-ResX=1280','-ResY=720','-nosplash','-stdout','-FullStdOutLogOutput','-ExecCmds=t.MaxFPS 60']
start=time.time()
with (out/'unreal-pause.log').open('w') as log:
    subprocess.run(args,cwd=root,env=dict(os.environ,SDL_VIDEODRIVER='offscreen'),stdout=log,stderr=subprocess.STDOUT,check=True,timeout=180)
log=(out/'unreal-pause.log').read_text();assert 'SM_NATIVE_PAUSE_PASS' in log and 'SM_AREA_VISUAL_READY seed=15092600 source=24 destination=11' in log
assert 'Vulkan' in log and 'SM_NATIVE_PAUSE_FAIL' not in log
saved=root/'SMUnreal/Saved/SMTests'
for old,new in [('native-pause-100.png','pause-map-unreal.png'),('native-pause-verification.json','pause-verification.json')]:
    p=saved/old;assert p.stat().st_mtime>=start;shutil.copy2(p,out/new)
assert json.loads((out/'pause-verification.json').read_text())['passed']
(out/'render-command.json').write_text(json.dumps(dict(args=args,nativeSha256=hashlib.sha256(lib.read_bytes()).hexdigest(),artificialAllMapReveal=False,fixture='Generated connection arrival then native pause; collision discovery separately verified in transitions suite'),indent=2)+'\n')
print('MAP_EXPLORATION_VULKAN_PASS')
