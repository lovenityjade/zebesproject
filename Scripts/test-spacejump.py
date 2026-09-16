#!/usr/bin/env python3
"""Real native inputs, plus original-mode parity against the previous library."""
import ctypes as C,hashlib,json,shutil,socket,tempfile
from pathlib import Path
assert socket.gethostname()=='gaming-pc'
root=Path(__file__).resolve().parent.parent
out=root/'Docs/Movement';out.mkdir(parents=True,exist_ok=True)
fixture=root/'Unreal/Saved/SMTests/HUD-all-items.sram';original=fixture.read_bytes()
rows=[];reference={};samples={}
with tempfile.TemporaryDirectory(prefix='SMTests-spacejump-') as tmp:
 for label,filename,assisted in [('baseline','libsm_native.before-spacejump.so',0),('original','libsm_native.so',0),('assisted','libsm_native.so',1)]:
  lib=C.CDLL(str(root/'Native/build'/filename));lib.sm_init.argtypes=[C.c_char_p,C.c_char_p];lib.sm_simulation_ram.restype=C.c_void_p;lib.sm_pixels.restype=C.c_void_p
  save=Path(tmp)/f'{label}.sram';shutil.copy2(fixture,save)
  assert lib.sm_init(bytes(root/'roms/Super Metroid (Japan, USA) (En,Ja).sfc'),bytes(save))
  lib.sm_set_widescreen(0);lib.sm_set_assisted_walljump(0)
  if label!='baseline':lib.sm_set_assisted_spacejump(assisted)
  def step(b=0):assert lib.sm_step(b)
  def word(a):return C.c_uint16.from_address(lib.sm_simulation_ram()+a)
  for _ in range(8500):
   f,s=lib.sm_frame(),lib.sm_state();step(8 if f>180 and f%120<2 and not 7<=s<=18 else 0)
   if lib.sm_state()==8:break
  for _ in range(440):step()
  equipment=word(0x9a2).value
  for case,interval in [('quick',20),('steady',36),('late',65),('held',0),('missing',20),('water_without_gravity',20)]:
   word(0x9a2).value=equipment
   if case=='missing':word(0x9a2).value&=~0x200
   if case=='water_without_gravity':word(0x9a2).value&=~0x20
   assert lib.sm_teleport(6 if case=='water_without_gravity' else 0)
   for _ in range(550):step()
   if case!='water_without_gravity':
    for _ in range(110):step(128)
    for _ in range(100):step()
   if case=='water_without_gravity':
    assert not word(0xa74).value&4, 'Gravity palette/physics still active'
    assert lib.sm_water_y()!=32767 and lib.sm_samus_y()-lib.sm_visual_state(2,0)>lib.sm_water_y(), 'Fixture is not submerged'
   trace=[];hashes=[]
   for t in range(240):
    jump=t>=10 and (case=='held' or t<30 or (t>=40 and (t-40)%interval<8))
    step(128|(256 if jump else 0))
    trace.append([lib.sm_samus_x(),lib.sm_samus_y(),word(0xb36).value,word(0xb2e).value,lib.sm_player_motion(0)])
    hashes.append(hashlib.sha256(C.string_at(lib.sm_simulation_ram(),131072)+C.string_at(lib.sm_pixels(),256*240*4)).hexdigest())
   boosts=sum(row[2]==1 and trace[i-1][2]==2 and row[4]==3 for i,row in enumerate(trace) if i)
   if label=='baseline':reference[case]=hashes
   elif label=='original':assert hashes==reference[case],('Original behavior changed',case)
   if case in ('held','missing','water_without_gravity'):assert boosts==0,(label,case,boosts)
   rows.append(dict(mode=label,case=case,boosts=boosts,minY=min(r[1] for r in trace)))
   samples[label+'-'+case]=trace
   (out/'spacejump-traces.json').write_text(json.dumps(samples)+'\n')
   print(rows[-1],flush=True)
  assert lib.sm_cpu_opcodes()==0;lib.sm_shutdown()
for case in ('quick','steady','late'):
 a=next(r['boosts'] for r in rows if r['mode']=='assisted' and r['case']==case)
 b=next(r['boosts'] for r in rows if r['mode']=='original' and r['case']==case)
 assert a>0 and a>=b,(case,a,b)
assert any(next(r['boosts'] for r in rows if r['mode']=='assisted' and r['case']==case)>next(r['boosts'] for r in rows if r['mode']=='original' and r['case']==case) for case in ('quick','steady','late'))
assert fixture.read_bytes()==original
(out/'spacejump-traces.json').write_text(json.dumps(samples)+'\n')
report=dict(passed=True,originalRamAndPixelsExact=True,originalFrames=1440,noHeldAutoJump=True,itemRequired=True,underwaterRestrictionPreserved=True,scenarios=rows)
(out/'spacejump-verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('SM_SPACEJUMP_PASS',flush=True)
