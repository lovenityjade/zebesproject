#!/usr/bin/env python3
"""Capture effective VARIA escape data before pending native activation."""
import json,os,subprocess,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
out=root/'escape/capture';out.mkdir(parents=True,exist_ok=True)
if len(sys.argv)==2:
 for i in range(3):subprocess.run([sys.executable,__file__,str(root),str(i)],check=True)
 raise SystemExit(0)
i=int(sys.argv[2]);sys.path[:0]=[str(root/'Randomizer'),str(root/'Randomizer/upstream')]
import sm_options,sm_native_world,sm_integration
original=sm_options.resolve
def resolve(request):
 r=original(request);o=r['options'];o.update(tourian='Vanilla' if i==0 else 'Disabled',escapeRando='on',removeEscapeEnemies='on' if i==0 else 'off')
 if i==2:o.update(minimizer='on',minimizerQty=60,areaRandomization='full',bossRandomization='on')
 return r
class Done(BaseException):pass
def capture(patcher,rules):
 from rom.rom_patches import getPatchSetsFromPatcherSettings
 p=patcher;s=p.settings;patches=[];plms=[]
 from sm_escape import capture_clock
 clock=capture_clock(patcher)
 def record(name,patchDict=None):patches.append(dict(name=name,data=patchDict[name] if patchDict else None))
 p.applyIPSPatch=record;p.applyEscapeAttributes(s['escapeAttr'],plms)
 doors=[]
 for d in s['doors']:
  a,b=d['transition']
  if not a.Escape and not b.Escape:continue
  doors.append(dict(**{k:v for k,v in d.items() if k!='transition'},source=a.Name,target=b.Name,sourceRoom=a.RoomInfo,targetRoom=b.RoomInfo,sourceEntry=a.EntryInfo,targetEntry=b.EntryInfo,sourceExit=a.ExitInfo,targetExit=b.ExitInfo))
 result=dict(clock=clock,seed=15093500+i,tourian=s['tourian'],area=s['area'],boss=s['boss'],attributes=s['escapeAttr'],doors=doors,patches=patches,plms=plms,patchSets=getPatchSetsFromPatcherSettings(s),settingsKeys=sorted(s))
 (out/f'seed-{i:02d}.json').write_text(json.dumps(result,indent=2,default=lambda o:o.Name if hasattr(o,'Name') else str(o))+'\n')
 print('ESCAPE_CAPTURE_PASS',i,len(doors),flush=True);raise Done()
sm_options.resolve=resolve;sm_native_world.capture=capture
try:sm_integration.generate_once(dict(seed=15093500+i,skill='regular',options=dict(maxDifficulty='hard')))
except Done:pass
