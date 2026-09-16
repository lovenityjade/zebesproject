#!/usr/bin/env python3
"""Render native hidden/visible objectives and original navigation in Unreal."""
import hashlib,json,os,shutil,subprocess,sys,time
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
assert subprocess.run(['pgrep','-x','SMUnreal'],stdout=subprocess.DEVNULL).returncode==1
out=root/'objectives-map';lib=root/'Native/build/libsm_native.so'
shutil.copy2(out/'libsm_native.so',lib.with_suffix('.so.next'));os.replace(lib.with_suffix('.so.next'),lib)
results=[]
for case in [1,2]:
    m=json.loads((root/f'objective-public-seeds/seed-{case:02d}.json').read_text())['manifest'];obj=m['nativeContext']['objectives']
    args=['./SMUnreal/Binaries/Linux/SMUnreal','/Engine/Maps/Entry','-SMObjectiveLogicTest','-SMObjectivePublicTest',f'-SMStartTest={case}',
          '-SMObjectivePauseTest','-SMWarmup=12000','-RenderOffscreen','-unattended','-windowed','-ResX=1280',
          '-ResY=720','-nosplash','-stdout','-FullStdOutLogOutput','-ExecCmds=t.MaxFPS 60']
    if case==1:args+=['-SMAreaTest','-SMAreaConnectionTest=24']
    start=time.time()
    with (out/f'unreal-{case:02d}.log').open('w') as log:
        subprocess.run(args,cwd=root,env=dict(os.environ,SDL_VIDEODRIVER='offscreen'),stdout=log,stderr=subprocess.STDOUT,check=True,timeout=180)
    log=(out/f'unreal-{case:02d}.log').read_text()
    assert 'SM_OBJECTIVE_PAUSE_PASS' in log and 'SM_NATIVE_PAUSE_FAIL' not in log and 'Vulkan' in log
    assert f"SM_OBJECTIVE_ORACLE_PASS seed={m['seed']} count={len(obj['goals'])} required={obj['required']}" in log
    saved=root/'SMUnreal/Saved/SMTests'
    for name in [f'objective-pause-{t:03d}.png' for t in [100,200,290,390,480,590]]+['objective-pause-verification.json','objective-oracle.json']:
        p=saved/name;assert p.stat().st_mtime>=start;shutil.copy2(p,out/f'{case:02d}-{name}')
    assert json.loads((out/f'{case:02d}-objective-pause-verification.json').read_text())['passed']
    results.append(dict(args=args,seed=m['seed'],fingerprint=m['sha256'],hidden=bool(obj['flags']&2)))
(out/'render-results.json').write_text(json.dumps(dict(cases=results,nativeSha256=hashlib.sha256(lib.read_bytes()).hexdigest(),
    gameSha256=hashlib.sha256((root/'SMUnreal/Binaries/Linux/SMUnreal').read_bytes()).hexdigest(),scope=__doc__),indent=2)+'\n')
print('OBJECTIVE_PAUSE_VULKAN_PASS',flush=True)
