#!/usr/bin/env python3
"""Actual native Bomb Torizo PLMs and native SPC objective sound priority."""
import os,sys,struct,wave
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
source=(root/'test-native-objectives.py').read_text().split("for goal in audit['goals']:")[0]
source=source.replace('objectives/libsm_native.so','objectives-map/libsm_native.so').replace("root/'objectives/native'","root/'objectives-map/dependencies'")
exec(compile(source,'objective-dependency-fixture','exec'))
world=json.loads((root/'Randomizer/native_world_data.json').read_text())
patch=next(p['id'] for p in world['patches'] if p['name']=='bomb_torizo.ips')
door=routine('PlmInstr_JumpIfSamusHasNoBombs',C.c_void_p,C.c_void_p,C.c_uint16)
statue=routine('PlmPreInstr_WakePlmIfSamusHasBombs',None,C.c_uint16)
delete=routine('PlmInstr_Delete',C.c_void_p,C.c_void_p,C.c_uint16)
cases=[]
for tweak,sleep,robots in itertools.product([False,True],[False,True],[False,True]):
    names=['activate chozo robots'] if robots else ['kill kraid']
    p=plan([byname[n] for n in names],flags=4 if sleep else 0)
    if sleep and robots:
        assert not config(0,C.byref(p),audit['sha256'].encode());continue
    ids=(C.c_uint8*int(tweak))(*([patch] if tweak else []))
    assert l.sm_start_configure(0,0,ids,len(ids),world['sha256'].encode())
    commit(p)
    for bombs,present in itertools.product([False,True],[False,True]):
        clear();put(0x9a4,0x1000 if bombs else 0);put(0x1c83,0xef83 if present else 0)
        put(0x1d27,0xd300);put(0xde1c,25);put(0x1cd7,0xd33b)
        target=(C.c_uint8*2)(0x34,0x92);answer=door(C.addressof(target),0)
        expected=(not sleep and not present) if tweak else bombs
        assert (answer==C.addressof(target)+2)==expected,(tweak,sleep,robots,bombs,present)
        statue(0);assert word(0x1d27)==(0xd302 if expected else 0xd300)
        assert word(0xde1c)==(1 if expected else 25) and word(0x1cd7)==(0xd356 if expected else 0xd33b)
        cases.append(dict(tweak=tweak,sleep=sleep,robots=robots,bombs=bombs,itemPresent=present,wake=expected))

# Pending A/B/C configurations change behavior only at a committed activation.
ids=(C.c_uint8*1)(patch)
for slot,flags in enumerate([4,0,4]):
    assert l.sm_start_configure(slot,0,ids,1,world['sha256'].encode())
    p=plan([byname['kill kraid']],flags=flags);assert configure(slot,[]) and config(slot,C.byref(p),audit['sha256'].encode())
for slot in [0,1,2,1,0]:
    assert activate(slot,1) and select(items,100,m['sha256'].encode());clear()
    put(0x1c83,0xef83);put(0x1d27,0xd300);put(0xde1c,25);put(0x1cd7,0xd33b)
    delete(None,76);assert not word(0x1c83);statue(0)
    assert word(0x1d27)==(0xd302 if slot==1 else 0xd300)
    before=state(6);assert activate((slot+1)%3,1);assert state(6)==before
    bad=(Item*100)(*items);bad[0].address=0;assert not select(bad,100,m['sha256'].encode()) and state(6)==before

# Decode the native SPC's compiled member map rather than hardcoding host
# offsets for its full C struct. This is the game's actual translated player.
size_line=next(line.split() for line in subprocess.check_output(['nm','-S',str(root/'objectives-map/libsm_native.so')],text=True).splitlines() if line.endswith(' kSpcPlayer_Maps'))
mapbytes=C.string_at(base+symbols['kSpcPlayer_Maps'],int(size_line[1],16))
members={spc:(off,size) for off,spc,size in struct.iter_unpack('<HHH',mapbytes)}
for address in [0,0x3f8,0x441,0x4bc]:assert address in members
create=routine('SpcPlayer_Create',C.c_void_p)
initialize=routine('SpcPlayer_Initialize',None,C.c_void_p)
upload=routine('SpcPlayer_Upload',None,C.c_void_p,C.c_void_p)
handle=routine('Sfx2_HandleCmdFromSnes',None,C.c_void_p)
samples=routine('SpcPlayer_GenerateSamples',None,C.c_void_p)
get_samples=routine('dsp_getSamples',None,C.c_void_p,C.c_void_p,C.c_int)
free_dsp=routine('dsp_free',None,C.c_void_p)
libc=C.CDLL(None);libc.free.argtypes=[C.c_void_p]
class SpcHead(C.Structure):_fields_=[('history',C.c_void_p),('timer',C.c_uint8),('dsp',C.c_void_p)]
sound_cases=[]
for mode in ['vanilla','historical','goals','hidden-goals']:
    if mode=='vanilla':assert activate(0,0) and select(None,0,None)
    elif mode=='historical':
        assert config(0,None,audit['sha256'].encode()) and activate(0,1) and select(items,100,m['sha256'].encode())
    else:commit(plan([byname['tickle the red fish']],flags=2 if mode=='hidden-goals' else 1))
    player=create();initialize(player);upload(player,romptr+0x278000)
    def byte(address):return C.c_uint8.from_address(player+members[address][0])
    def command(sound):C.c_uint8.from_address(player+members[0][0]+2).value=sound;handle(player)
    command(0x19)
    assert byte(0x441).value==2
    priority=int(mode in ['goals','hidden-goals']);assert byte(0x4bc).value==priority
    pcm=(C.c_int16*(736*2))();dsp=SpcHead.from_address(player).dsp;audio=bytearray()
    for _ in range(4):samples(player);get_samples(dsp,pcm,736);audio+=bytes(pcm)
    command(6);current=byte(0x3f8).value
    assert current==(0x19 if priority else 6),(mode,current)
    for _ in range(24):samples(player);get_samples(dsp,pcm,736);audio+=bytes(pcm)
    assert any(audio),'SPC emitted only silence'
    with wave.open(str(root/'objectives-map/dependencies'/f'{mode}.wav'),'wb') as wav:
        wav.setnchannels(2);wav.setsampwidth(2);wav.setframerate(44100);wav.writeframes(audio)
    sound_cases.append(dict(mode=mode,voices=2,priority=priority,afterCompetingSound=current,audioSha256=hashlib.sha256(audio).hexdigest()))
    free_dsp(dsp);libc.free(player)
assert l.sm_cpu_opcodes()==0
(root/'objectives-map/dependencies-results.json').write_text(json.dumps(dict(passed=True,bombTorizo=cases,
    pickupDeletion=True,mixedSlots=True,pendingRollback=True,spc=sound_cases,cpuOpcodes=0,
    nativeSha256=hashlib.sha256((root/'objectives-map/libsm_native.so').read_bytes()).hexdigest()),indent=2)+'\n')
l.sm_shutdown();print('OBJECTIVE_DEPENDENCIES_NATIVE_PASS',flush=True)
