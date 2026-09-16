#!/usr/bin/env python3
"""Isolated gaming-pc credit roll, statistics, rendering and preview recovery."""
from pathlib import Path
fixture=Path(__file__).with_name('test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
exec(compile(fixture,'tracker-fixture','exec'))
out=root/'credits-results';out.mkdir(exist_ok=True)
l.sm_credits_state.argtypes=[C.c_int];l.sm_stats_value.argtypes=[C.c_int];l.sm_stats_value.restype=C.c_uint64
l.sm_stats_partial.restype=C.c_int
l.sm_slots_copy_sram.argtypes=[C.c_void_p,C.c_int]
def sram():
 b=C.create_string_buffer(8192);assert l.sm_slots_copy_sram(b,8192)==8192;return b.raw
boot('SMTests-credits',True)
# Actual allowed native shots, including held-button autorepeat.
for _ in range(20):step(64)
wait(30)
start=l.sm_stats_value(31)
print('shoot binding',hex(word(0x9b2)),flush=True)
for _ in range(180):step(512)
print('initial shot delta',l.sm_stats_value(31)-start,flush=True)
assert l.sm_stats_value(31)>start
# First preview cannot modify even a byte of game RAM/SRAM or the native frame clock.
before=read(0,131072),sram(),l.sm_frame(),tuple(l.sm_stats_value(i) for i in range(48))
assert l.sm_credits_launch() and l.sm_credits_state(0)==2
seen={}
for _ in range(2000):
 step(128)
 section=l.sm_credits_state(4)
 if section not in seen and l.sm_credits_state(1)//16>= [25,50,80,325,450][min(section,4)]:
  capture('section-'+str(section));seen[section]=l.sm_credits_state(1)
 if l.sm_credits_state(0)==0:break
 if l.sm_credits_state(1)==256:capture('project')
 if l.sm_credits_state(1)==600:capture('decompilation')
 if l.sm_credits_state(1)==3600:capture('varia-staff')
 if l.sm_credits_state(1)==6000:capture('stats')
assert l.sm_credits_state(0)==0
assert before==(read(0,131072),sram(),l.sm_frame(),tuple(l.sm_stats_value(i) for i in range(48)))
assert l.sm_credits_launch();wait(220);capture('preview-stars-a');wait(70);capture('preview-stars-b')
assert l.sm_credits_state(0)==2
step(8);assert l.sm_credits_state(0)==0
assert before==(read(0,131072),sram(),l.sm_frame(),tuple(l.sm_stats_value(i) for i in range(48)))
# Resume original game and its native frame clock.
step();assert l.sm_frame()==before[2]+1 and l.sm_state()==8
assert l.sm_cpu_opcodes()==0
l.sm_set_widescreen(0);assert l.sm_credits_launch();wait(256)
Image.frombytes('RGBA',(256,240),C.string_at(l.sm_pixels(),256*240*4),'raw','BGRA').crop((0,0,256,224)).resize((768,672),Image.Resampling.NEAREST).save(out/'preview-4x3.png')
l.sm_credits_close();l.sm_set_widescreen(1)
assert l.sm_save()
assert (out/'SMTests-credits'/m['sha256']/'sram.dat.stats').exists()
report=dict(passed=True,previewRamSramClockAndStatsExact=True,previewReturnsByStartAndCompletion=True,sections=seen,sourceRomUnchanged=hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash,nativeCpuOpcodes=0,nativeSha256=hashlib.sha256((root/'Native/build/libsm_native.so').read_bytes()).hexdigest())
l.sm_shutdown()
(out/'preview-verification.json').write_text(json.dumps(report,indent=2)+'\n')
print('CREDITS PREVIEW PASS',flush=True)
