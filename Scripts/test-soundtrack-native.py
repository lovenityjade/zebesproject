#!/usr/bin/env python3
"""Real native frames, queue transitions, mode changes and preview recovery."""
from pathlib import Path
fixture=Path(__file__).with_name('test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
exec(compile(fixture,'tracker-fixture','exec'))
import wave
l.sm_soundtrack_configure.argtypes=[C.c_int,C.c_char_p]
l.sm_soundtrack_status.argtypes=[C.c_int];l.sm_soundtrack_position.restype=C.c_uint64
l.sm_audio.restype=C.c_void_p
music=root/'Soundtracks/Remastered';manifest=json.loads((music/'manifest.json').read_text())
for track in manifest['tracks']:assert hashlib.sha256((music/track['file']).read_bytes()).hexdigest()==track['sha256']
l.sm_soundtrack_configure(1,str(music).encode())
boot('SMTests-soundtrack',False)
print('Gameplay',hex(l.sm_room()),'track',l.sm_soundtrack_status(1),flush=True)
assert l.sm_soundtrack_status(1)>0
out=root/'soundtrack-tests';out.mkdir(exist_ok=True)
def audio_bytes():return C.string_at(l.sm_audio(),736*4)
def queue(bank,command):
 # Populate the original native queue: stop, upload bank, then select cue.
 for i in range(8):put(0x619+i*2,0);put(0x629+i*2,0)
 put(0x639,6);put(0x63b,0);put(0x63d,0);put(0x63f,0)
 for i,v in enumerate((0,0xff00|bank,command)):put(0x619+i*2,v);put(0x629+i*2,8)
 wait(80)
results=[]
for bank,cmd,expected in [(3,5,4),(3,6,5),(6,5,6),(6,7,7),(9,5,8),(9,6,9),(12,5,10),(15,5,11),(18,5,12),(21,5,13),(24,5,14),(27,5,15),(27,6,16),(30,5,17),(33,5,18),(36,5,19),(36,6,21),(36,7,20),(39,5,22),(39,6,23),(42,5,24),(45,6,25),(48,5,26),(48,6,27),(54,5,28),(57,5,29),(60,5,30),(63,5,100),(66,5,101),(15,1,1),(15,2,2),(15,3,3)]:
 queue(bank,cmd);actual=l.sm_soundtrack_status(1);assert actual==expected,(bank,cmd,expected,actual)
 results.append({'bank':bank,'command':cmd,'track':actual,'frame':l.sm_frame()})
queue(15,5)
# Mode switch must not change gameplay RAM, SRAM, save identity or frame clock.
before=read(0,131072),l.sm_frame()
l.sm_soundtrack_configure(0,str(music).encode());assert l.sm_soundtrack_status(0)==0 and l.sm_soundtrack_status(1)==0
assert before==(read(0,131072),l.sm_frame())
original=[]
for _ in range(120):step();original.append(audio_bytes())
assert any(b'\x00'*len(b)!=b for b in original)
l.sm_soundtrack_configure(1,str(music).encode());assert l.sm_soundtrack_status(1)==11
remastered=[]
for _ in range(120):step();remastered.append(audio_bytes())
assert b''.join(original)!=b''.join(remastered)
for label,blocks in [('original',original),('remastered',remastered)]:
 with wave.open(str(out/(label+'.wav')),'wb') as f:f.setnchannels(2);f.setsampwidth(2);f.setframerate(44100);f.writeframes(b''.join(blocks))
# SFX preserve the original native command path and remain audible alongside PCM.
wait(120)
import struct
pcm=(music/'zebes-11.pcm').read_bytes();loop=int.from_bytes(pcm[4:8],'little');total=(len(pcm)-8)//4
residual_frames=0
for _ in range(45):
 pos=l.sm_soundtrack_position();step(512);expected=bytearray()
 for i in range(736):
  frame=pos+i
  if frame>=total:frame=loop+(frame-total)%(total-loop)
  expected.extend(pcm[8+frame*4:12+frame*4])
 if audio_bytes()!=expected:residual_frames+=1
assert residual_frames>5,('SPC firing effects missing',residual_frames)
# A debug credit preview uses its own cursor and leaves gameplay intact.
before=read(0,131072),l.sm_frame();pos=l.sm_soundtrack_position();track=l.sm_soundtrack_status(1)
assert l.sm_credits_launch();assert l.sm_soundtrack_status(1)==30
wait(100);assert l.sm_soundtrack_position()==100*736
l.sm_credits_close();assert before==(read(0,131072),l.sm_frame());assert pos==l.sm_soundtrack_position() and track==l.sm_soundtrack_status(1)
# Live missing-file fallback via a separate empty directory does not modify installed audio.
empty=out/'empty';empty.mkdir(exist_ok=True)
l.sm_soundtrack_configure(1,str(empty).encode());assert l.sm_soundtrack_status(3)==1
wait(120);assert any(audio_bytes())
l.sm_soundtrack_configure(1,str(music).encode());assert l.sm_soundtrack_status(1)==11
# Render the credit section with the actual ROM font for visual review.
assert l.sm_credits_launch()
for _ in range(1500):
 step(128)
 frame=l.sm_credits_state(1)
 if frame in (5056,5440,5824,6208,6592,6976):capture('music-credits-'+str(frame))
 if l.sm_credits_state(0)==0:break
l.sm_credits_close()
assert l.sm_cpu_opcodes()==0
l.sm_shutdown();assert l.sm_soundtrack_status(1)==0
assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
(out/'native-result.json').write_text(json.dumps({'passed':True,'cues':results,'previewPreservesRamAndCursor':True,'togglePreservesRam':True,'romUnchanged':True,'nativeCpuOpcodes':0,'framesWithSpcEffects':residual_frames},indent=2))
print('PASS: 32 real queue cues, Original/Remastered switching, missing-pack fallback, preview restoration, shutdown, audio capture',flush=True)
