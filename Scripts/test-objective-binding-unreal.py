#!/usr/bin/env python3
"""Read and apply an objective-bearing manifest in the actual Unreal runtime."""
import hashlib,json,os,shutil,subprocess,sys,time
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
assert subprocess.run(['pgrep','-x','SMUnreal'],stdout=subprocess.DEVNULL).returncode==1
out=root/'objectives-binding';lib=root/'Native/build/libsm_native.so'
shutil.copy2(out/'libsm_native.so',lib.with_suffix('.so.next'));os.replace(lib.with_suffix('.so.next'),lib)
manifest=json.loads((root/'objective-seeds/seed-01.json').read_text())['manifest']
args=['./SMUnreal/Binaries/Linux/SMUnreal','/Engine/Maps/Entry','-SMObjectiveTest','-SMAreaTest','-SMStartTest=1','-SMAreaConnectionTest=24','-SMPauseTest','-SMWarmup=12000','-RenderOffscreen','-unattended','-windowed','-ResX=1280','-ResY=720','-nosplash','-stdout','-FullStdOutLogOutput','-ExecCmds=t.MaxFPS 60']
start=time.time()
with (out/'unreal-pause.log').open('w') as log:
 subprocess.run(args,cwd=root,env=dict(os.environ,SDL_VIDEODRIVER='offscreen'),stdout=log,stderr=subprocess.STDOUT,check=True,timeout=180)
log=(out/'unreal-pause.log').read_text()
assert 'SM_NATIVE_PAUSE_PASS' in log and 'SM_NATIVE_PAUSE_FAIL' not in log and 'Vulkan' in log
assert f"SM_OBJECTIVES_VISUAL_READY seed={manifest['seed']} count=4 required=4" in log
assert f"SM_AREA_VISUAL_READY seed={manifest['seed']} source=24 destination={manifest['nativeContext']['topology']['nativeAreas']['destinations'][24]}" in log
saved=root/'SMUnreal/Saved/SMTests'
for old,new in [('native-pause-100.png','pause-map-unreal.png'),('native-pause-verification.json','pause-verification.json')]:
 p=saved/old;assert p.stat().st_mtime>=start;shutil.copy2(p,out/new)
assert json.loads((out/'pause-verification.json').read_text())['passed']
(out/'render-results.json').write_text(json.dumps(dict(args=args,seed=manifest['seed'],fingerprint=manifest['sha256'],nativeSha256=hashlib.sha256(lib.read_bytes()).hexdigest(),gameSha256=hashlib.sha256((root/'SMUnreal/Binaries/Linux/SMUnreal').read_bytes()).hexdigest(),scope='Effective objective manifest parsed and applied; original area arrival/pause/navigation/resume, not the future objectives screen'),indent=2)+'\n')
print('OBJECTIVE_BINDING_VULKAN_PASS',flush=True)
