#!/usr/bin/env python3
"""Source pose tables, original movement and pickup/music C routines."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
fixture=(root/'gameplay/test-native-gameplay-patches.py').read_text().split('assert l.sm_seed_rules_capabilities()')[0]
fixture=fixture.replace('gameplay/libsm_native.so','movement-pickups/libsm_native.so').replace("root/'gameplay/native'","root/'movement-pickups/native'")
exec(compile(fixture,'movement-pickup-fixture','exec'))
assert l.sm_seed_rules_capabilities()==16383
audit=json.loads((root/'movement-pickups/source-data.json').read_text())
lookup=routine('Samus_LookupTransitionTable',None)
spin=routine('HandleJumpTransition_SpinJump',None)
move=routine('Samus_Movement_03_SpinJumping',None)
romptr=C.c_void_p.from_address(base+symbols['g_rom']).value
report={};count=0
# Source table entries are decoded from pinned IPS into the separate audit.
# Check normal and remapped jump controls through the real transition lookup.
for swapped in [False,True]:
 for row in audit['poses']:
  for held in [0x80,0x180,0x280,0x480,0x880,0x40,0x140,0x240,0x8080,0xb0]:
   for fresh in [0,held&0x80]:
    matches=[e for e in row['entries'] if e[0]&~fresh==0 and e[1]&~held==0]
    if not matches:continue # no-match behavior is the unchanged original path
    reset(2048);setv('samus_pose',row['pose']);setv('samus_new_pose',65535)
    for name,mask in [('shoot_x',0x40),('jump_a',0x8000 if swapped else 0x80),('run_b',0x80 if swapped else 0x8000),('itemcancel_y',0x4000),('aim_up_R',0x20),('aim_down_L',0x10)]:setv('button_config_'+name,mask)
    def physical(mask):return (mask&~0x8080)|((mask&0x80)<<8)|((mask&0x8000)>>8) if swapped else mask
    setv('joypad1_newkeys',physical(fresh));setv('joypad1_lastkeys',physical(held));lookup()
    wanted=matches[0][2];assert getv('samus_new_pose')==(65535 if wanted==row['pose'] else wanted),(row['pose'],held,fresh,swapped)
    count+=1
report['respinTransitions']=count
for randomized in [True,False]:
 assert select(items,100,m['sha256'].encode()) if randomized else select(None,0,None)
 for flags in [0,2048]:
  for prev in [0,2,3,6,20]:
   reset(flags);C.memmove(ram+offsets['samus_prev_movement_type2'],bytes([prev]),1)
   setv('samus_y_speed',7);setv('samus_y_subspeed',123);setv('samus_y_dir',2)
   spin();unchanged=prev in ([2,3,6,20] if randomized and flags else [3,20])
   assert (getv('samus_y_speed')==7 and getv('samus_y_subspeed')==123 and getv('samus_y_dir')==2)==unchanged
assert select(items,100,m['sha256'].encode())
cases=[]
for flags,assist in [(0,0),(4096,0),(4096,1)]:
 for liquid in [0,1]:
  for speed in [0,0x8000,3<<16,6<<16]:
   for direction,equipment,press in [(2,512,128),(1,512,128),(2,0,128),(2,512,0)]:
    reset(flags);l.sm_set_assisted_spacejump(assist)
    for n,v in [('equipped_items',equipment),('samus_suit_palette_index',0),('fx_y_pos',65535),('lava_acid_y_pos',65535),('samus_pose',0x1b),('samus_y_dir',direction),('samus_y_speed',speed>>16),('samus_y_subspeed',speed&65535),('liquid_physics_type',liquid),('joypad1_lastkeys',128),('joypad1_newkeys',press),('button_config_jump_a',128),('UNUSED_word_7E0DFA',0),('samus_x_pos',128),('samus_y_pos',128),('room_width_in_blocks',16),('room_height_in_blocks',16)]:setv(n,v)
    C.memset(ram+0x10002,0,8192)
    move();qualified=equipment and direction==2 and (flags or (0x80 if liquid else 0x280)<=speed>>8<0x500)
    assert bool(getv('UNUSED_word_7E0DFA')&1)==bool(qualified),(flags,assist,liquid,speed,direction,equipment,press)
    if qualified and press:assert getv('samus_y_dir')==1
    cases.append([flags,assist,liquid,speed,direction,equipment,press,bool(qualified)])
report['spaceJump']=cases
pickup=routine('PlmInstr_ClearMusicQueueAndQueueTrack',C.c_void_p,C.c_void_p,C.c_uint16)
resume=routine('PlayRoomMusicTrackAfterAFrames',None,C.c_uint16)
for row in audit['sounds']:
 reset(8192);setv('game_state',8);setv('debug_disable_sounds',0);setv('power_bomb_explosion_status',0)
 C.memset(ram+0x643,0,0x43);C.memset(ram+0x619,0x55,32)
 put(0x639,2);put(0x63b,0);put(0x63f,10)
 music=read(0x619,42);arg=romptr+0x20000+row['address']
 assert pickup(arg,0)==arg+1
 assert read(0x619,42)==music and getv('debug_saved_yscroll')==2
 assert read(0x656+(row['group']-1)*16,1)==bytes([row['sound']]),row
 resume(8);assert getv('debug_saved_yscroll')==0 and read(0x619,42)==music
report['pickupSounds']=len(audit['sounds'])
# Unlisted PLMs still clear/start music and restore it later, as NORMAL does.
reset(8192);arg=C.create_string_buffer(b'\x02');pickup(C.addressof(arg),0)
assert getv('debug_saved_yscroll')==1
resume(8);assert getv('debug_saved_yscroll')==0
interaction=routine('HandleMessageBoxInteraction_Async',C.c_uint8)
for flags in [0,8192]:
 reset(flags);C.c_uint8.from_address(ram+offsets['coroutine_state_4']).value=0
 setv('message_box_index',3);interaction()
 assert getv('my_counter')==(32 if flags else 360)
report['messageDelay']=[360,32]
assert l.sm_cpu_opcodes()==0;l.sm_shutdown()
report.update(cpu=0,nativeSha256=hashlib.sha256(libpath.read_bytes()).hexdigest())
(root/'movement-pickups/native-results.json').write_text(json.dumps(report,indent=2)+'\n')
print('NATIVE_MOVEMENT_PICKUPS_PASS',count,len(cases),flush=True)
