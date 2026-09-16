#!/usr/bin/env python3
"""Game Over actions, custom music and surviving native Metroid audio."""
from pathlib import Path
fixture=Path(__file__).with_name('test-live-tracker-native.py').read_text().split('palette=[0x3f3f3f')[0]
exec(compile(fixture,'tracker-fixture','exec'))
import wave
l.sm_audio.restype=C.c_void_p
l.sm_gameover_labels.restype=C.c_void_p
l.sm_soundtrack_gameover_configure.argtypes=[C.c_char_p]
l.sm_soundtrack_configure.argtypes=[C.c_int,C.c_char_p]
results=root/'gameover-tests';results.mkdir(exist_ok=True)
track=root/'SMUnreal/Content/GameOver/GameOver.pcm'
assert track.read_bytes()[:4]==b'MSU1'
silent=results/'silent.pcm';silent.write_bytes(b'MSU1'+bytes(4)+bytes(44100*4))
for remastered in (0,1):
 l.sm_soundtrack_configure(remastered,str(root/'Soundtracks/Remastered').encode())
 for action in ('continue','end'):
  boot(f'SMTests-gameover-{remastered}-{action}',False)
  l.sm_soundtrack_gameover_configure(str(track).encode())
  assert l.sm_test_gameover()
  wait(200)
  assert l.sm_state()==26 and l.sm_gameover_state(2)==4
  assert l.sm_soundtrack_gameover_status()==1
  labels=C.string_at(l.sm_gameover_labels(),128*48*4)
  assert any(labels) and any(C.string_at(l.sm_audio(),736*4))
  assert l.sm_gameover_state(1)==0
  if action=='end':press(32)
  assert l.sm_gameover_state(1)==int(action=='end')
  if action=='end':assert labels!=C.string_at(l.sm_gameover_labels(),128*48*4)
  press(256) # Native A / confirm.
  states=set()
  for _ in range(350):step();states.add(l.sm_state())
  assert (5 in states or 31 in states) if action=='continue' else any(s in states for s in (0,1))
  assert l.sm_soundtrack_gameover_status()==0 and l.sm_cpu_opcodes()==0
  l.sm_shutdown()
  print('GAMEOVER ACTION PASS',remastered,action,sorted(states),flush=True)
# Silent replacement must leave the Metroid cries (SPC sound effects) audible.
boot('SMTests-gameover-metroid-audio',False)
l.sm_soundtrack_gameover_configure(str(silent).encode());assert l.sm_test_gameover();wait(250)
chunks=[]
for _ in range(480):step();chunks.append(C.string_at(l.sm_audio(),736*4))
assert l.sm_soundtrack_gameover_status()==1 and any(b for block in chunks for b in block),'Native Metroid SFX were muted'
with wave.open(str(results/'metroid-only.wav'),'wb') as wav:
 wav.setnchannels(2);wav.setsampwidth(2);wav.setframerate(44100);wav.writeframes(b''.join(chunks))
l.sm_shutdown()
assert hashlib.sha256(rom.read_bytes()).hexdigest()==rom_hash
print('GAMEOVER NATIVE PASS: both actions, Original/Remastered, Metroid audio, ROM unchanged',flush=True)
