#!/usr/bin/env python3
"""Native objective progress through Unreal's real async tracker, plus pause QA."""
import hashlib,json,os,shutil,subprocess,sys,time
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
assert subprocess.run(['pgrep','-x','SMUnreal'],stdout=subprocess.DEVNULL).returncode==1
out=root/'objectives-logic';lib=root/'Native/build/libsm_native.so'
shutil.copy2(out/'libsm_native.so',lib.with_suffix('.so.next'));os.replace(lib.with_suffix('.so.next'),lib)
m=json.loads((root/'objective-logic-seeds/seed-01.json').read_text())['manifest'];obj=m['nativeContext']['objectives']
args=['./SMUnreal/Binaries/Linux/SMUnreal','/Engine/Maps/Entry','-SMObjectiveLogicTest','-SMAreaTest','-SMStartTest=1','-SMAreaConnectionTest=24','-SMPauseTest','-SMWarmup=12000','-RenderOffscreen','-unattended','-windowed','-ResX=1280','-ResY=720','-nosplash','-stdout','-FullStdOutLogOutput','-ExecCmds=t.MaxFPS 60']
start=time.time()
with (out/'unreal-pause.log').open('w') as log:
 subprocess.run(args,cwd=root,env=dict(os.environ,SDL_VIDEODRIVER='offscreen'),stdout=log,stderr=subprocess.STDOUT,check=True,timeout=180)
log=(out/'unreal-pause.log').read_text()
assert 'SM_NATIVE_PAUSE_PASS' in log and 'SM_NATIVE_PAUSE_FAIL' not in log and 'Vulkan' in log
assert f"SM_OBJECTIVE_ORACLE_PASS seed={m['seed']} count={len(obj['goals'])} required={obj['required']}" in log
assert f"SM_AREA_VISUAL_READY seed={m['seed']} source=24 destination={m['nativeContext']['topology']['nativeAreas']['destinations'][24]}" in log
saved=root/'SMUnreal/Saved/SMTests'
for old,new in [('native-pause-100.png','pause-map-unreal.png'),('native-pause-verification.json','pause-verification.json'),('objective-oracle.json','objective-oracle.json')]:
 p=saved/old;assert p.stat().st_mtime>=start;shutil.copy2(p,out/new)
assert json.loads((out/'pause-verification.json').read_text())['passed']
oracle=json.loads((out/'objective-oracle.json').read_text())
assert [g['name'] for g in oracle['goals']]==obj['names']
assert oracle['progressSource']=='native-evaluator' and oracle['hidden'] and not oracle['revealed']
(out/'render-results.json').write_text(json.dumps(dict(args=args,seed=m['seed'],fingerprint=m['sha256'],nativeSha256=hashlib.sha256(lib.read_bytes()).hexdigest(),gameSha256=hashlib.sha256((root/'SMUnreal/Binaries/Linux/SMUnreal').read_bytes()).hexdigest(),scope=__doc__),indent=2)+'\n')
print('OBJECTIVE_LOGIC_VULKAN_PASS',flush=True)
