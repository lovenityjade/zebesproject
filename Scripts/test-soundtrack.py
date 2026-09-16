#!/usr/bin/env python3
"""Focused MSU PCM/mapping/mode tests; run on isolated gaming-pc only."""
import ctypes as C,json,os,struct,subprocess,sys,tempfile
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').exists()
out=root/'soundtrack-tests';out.mkdir(exist_ok=True)
stub=out/'spc-stub.c';stub.write_text('#include <stdint.h>\nint last_spc=-1; void sm_soundtrack_spc_command(uint8_t c){last_spc=c;}\n')
subprocess.run(['gcc','-shared','-fPIC','-Wall','-Wextra','-Werror','-o',str(out/'pcm.so'),str(root/'Native/sm_soundtrack.c'),str(stub)],check=True)
l=C.CDLL(str(out/'pcm.so'))
l.sm_soundtrack_configure.argtypes=[C.c_int,C.c_char_p]
l.sm_soundtrack_command.argtypes=[C.c_uint,C.c_uint8];l.sm_soundtrack_command.restype=C.c_uint8
l.sm_soundtrack_position.restype=C.c_uint64
l.sm_soundtrack_mix.argtypes=[C.POINTER(C.c_int16),C.c_int]
l.sm_soundtrack_preview_mix.argtypes=l.sm_soundtrack_mix.argtypes
spc=C.c_int.in_dll(l,'last_spc')
with tempfile.TemporaryDirectory(prefix='pcm-',dir=out) as tmp:
 p=Path(tmp)
 def pcm(n,loop=1,values=(1000,-1000,2000,-2000,3000,-3000)):
  (p/f'zebes-{n}.pcm').write_bytes(b'MSU1'+struct.pack('<I',loop)+struct.pack('<'+'h'*len(values),*values))
 for n in list(range(1,31))+[100,101]:pcm(n)
 l.sm_soundtrack_configure(1,str(p).encode())
 mapping=[[4,5],[4,5],[6,0,7],[8,9],[10],[11],[12],[13],[14],[15,16],[17,0],[18],[19,21,20],[22,23],[24],[0,25,0],[26,27],[0,0,0],[28],[29],[30],[100],[101],[22,23],[10]]
 for bank,tracks in enumerate(mapping):
  for command,track in enumerate(tracks,5):
   l.sm_soundtrack_command(0,0)
   result=l.sm_soundtrack_command(bank*3,command)
   assert l.sm_soundtrack_status(1)==track,(bank,command,track)
   assert result==(0 if track else command)
 for command in (1,2,3):assert l.sm_soundtrack_command(0,command)==0 and l.sm_soundtrack_status(1)==command
 l.sm_soundtrack_command(15,5)
 audio=(C.c_int16*10)(*([100]*10));l.sm_soundtrack_mix(audio,5)
 assert list(audio)==[1100,-900,2100,-1900,3100,-2900,2100,-1900,3100,-2900]
 pos=l.sm_soundtrack_position();l.sm_soundtrack_command(15,5);assert l.sm_soundtrack_position()==pos
 assert l.sm_soundtrack_command(15,4)==4 and l.sm_soundtrack_status(1)==11
 l.sm_soundtrack_configure(0,str(p).encode());assert spc.value==5 and not l.sm_soundtrack_status(1)
 audio=(C.c_int16*2)(123,-456);l.sm_soundtrack_mix(audio,1);assert list(audio)==[123,-456]
 l.sm_soundtrack_configure(1,str(p).encode());assert spc.value==0 and l.sm_soundtrack_status(1)==11
 l.sm_soundtrack_preview_begin();assert l.sm_soundtrack_status(1)==30
 preview=(C.c_int16*8)(*([99]*8));l.sm_soundtrack_preview_mix(preview,4);assert list(preview)==[1000,-1000,2000,-2000,3000,-3000,0,0]
 l.sm_soundtrack_preview_end();assert l.sm_soundtrack_status(1)==11 and l.sm_soundtrack_position()==0
 for n,bank,command in ((1,0,1),(2,0,2),(29,57,5),(30,60,5),(100,63,5),(101,66,5)):
  l.sm_soundtrack_command(bank,command);audio=(C.c_int16*10)();l.sm_soundtrack_mix(audio,5);assert list(audio)[6:]==[0]*4
  assert not l.sm_soundtrack_status(2);l.sm_soundtrack_command(bank,command);assert not l.sm_soundtrack_status(2)
 l.sm_soundtrack_command(0,0);pcm(11,loop=999)
 l.sm_soundtrack_command(15,5);audio=(C.c_int16*8)();l.sm_soundtrack_mix(audio,4);assert list(audio)[6:]==[1000,-1000]
 l.sm_soundtrack_command(0,0);pcm(11,values=(32767,-32768));l.sm_soundtrack_command(15,5)
 audio=(C.c_int16*2)(32000,-32000);l.sm_soundtrack_mix(audio,1);assert list(audio)==[32767,-32768]
 for bad in (b'not pcm',b'MSU1'+bytes(4)+bytes(3)):
  l.sm_soundtrack_command(0,0);(p/'zebes-11.pcm').write_bytes(bad);assert l.sm_soundtrack_command(15,5)==5 and l.sm_soundtrack_status(3)
 l.sm_soundtrack_command(0,0);(p/'zebes-11.pcm').unlink();assert l.sm_soundtrack_command(15,5)==5
 l.sm_soundtrack_reset();assert not l.sm_soundtrack_status(1) and not l.sm_soundtrack_status(4)
print('PASS: all bank/cue mappings, signed PCM mix, saturation, exact loop, EOF, duplicate requests, ambience, toggle, preview isolation, malformed/missing tracks')
(out/'unit-result.json').write_text(json.dumps({'passed':True,'mappingBanks':25,'voiceTracks':[100,101]}))
