#!/usr/bin/env python3
"""Targeted original C routines; isolated gaming-pc, no user save changes."""
import os,sys
from pathlib import Path
root=Path(sys.argv[1]).resolve()
assert os.uname().nodename=='gaming-pc' and (root/'ISOLATED_TEST_DIRECTORY').is_file()
(root/'gameplay').mkdir(exist_ok=True)
fixture=(root/'test-native-tweaks.py').read_text().split('for slot,names in enumerate')[0]
fixture=fixture.replace('world-data/libsm_native.so','gameplay/libsm_native.so').replace("root/'tweaks/native'","root/'gameplay/native'")
exec(compile(fixture,'gameplay-fixture','exec'))
import re
variables=(root/'gameplay/variables.h').read_text()
offsets={n:int(a,16) for n,a in re.findall(r'#define (\w+) .*?g_ram\+0x([0-9A-F]+)',variables)}
def setv(n,v):put(offsets[n],v)
def getv(n):return word(offsets[n])
def reset(flags):
 C.memmove(ram,baseline,len(baseline));l.sm_seed_rules_configure(flags)
assert l.sm_seed_rules_capabilities()==511
cf=routine('Hdmaobj_CrystalFlash',C.c_uint8)
cfmain=routine('SamusMoveHandler_CrystalFlashMain',None)
landing=routine('Samus_HandleTransFromBlockColl_1_0',C.c_uint8)
original=routine('sm_original_landing_transition',C.c_uint8)
fire=routine('FireChargedBeam',None);sba=routine('FireSba',C.c_uint8)
results={}
# Real entry eligibility: original 10/10/10 restriction, patched zero threshold;
# health/reserve/vertical speed restrictions must remain effective.
entry=[]
for flags,ammo,health,reserve,vy,wanted in [(0,1,40,0,0,1),(64,1,40,0,0,0),(64,1,51,0,0,1),(64,1,40,1,0,1),(64,1,40,0,1,1),(0,10,40,0,0,0)]:
 reset(flags)
 for n,v in [('game_state',8),('joypad1_lastkeys',getv('button_config_shoot_x')|0x430),('samus_y_speed',vy),('samus_y_subspeed',0),('samus_health',health),('samus_reserve_health',reserve),('samus_missiles',ammo),('samus_super_missiles',ammo),('samus_power_bombs',ammo)]:setv(n,v)
 assert cf()==wanted,(flags,ammo,health,reserve,vy)
 if not wanted:assert getv('substate')==(30 if flags else 10)
 entry.append([flags,ammo,health,reserve,vy,wanted])
results['crystalFlashEntry']=entry
# Source ordering starts at Supers, skips depleted types, uses at most 30
# ammo, every eighth frame, and stops even with unused health capacity.
traces=[]
for ammo in [(2,1,2),(40,0,0),(0,4,0)]:
 reset(64)
 for n,v in zip(['samus_missiles','samus_super_missiles','samus_power_bombs'],ammo):setv(n,v)
 for n,v in [('substate',30),('which_item_to_pickup',0),('samus_health',40),('samus_max_health',2099)]:setv(n,v)
 before=[getv(n) for n in ['samus_missiles','samus_super_missiles','samus_power_bombs']]
 setv('nmi_frame_counter_word',1);cfmain();assert getv('samus_health')==40
 consumed=[]
 for _ in range(min(sum(ammo),30)):
  setv('nmi_frame_counter_word',8);cfmain()
  after=[getv(n) for n in ['samus_missiles','samus_super_missiles','samus_power_bombs']]
  consumed.append(next(i for i in range(3) if before[i]-after[i]==1));before=after
 cfmain();assert getv('samus_movement_handler')==0xd75b
 assert getv('samus_health')==40+50*min(sum(ammo),30)
 assert consumed==([1,2,0,2,0] if ammo==(2,1,2) else [0]*30 if ammo==(40,0,0) else [1]*4),consumed
 traces.append(dict(ammo=ammo,consumed=consumed))
results['crystalFlashConsumption']=traces
# Landing probes include subpixel momentum, wall/spin jumps, shooting and
# no-shot-direction poses (the latter are intentionally unpatched upstream).
romptr=C.c_void_p.from_address(base+symbols['g_rom']).value
poseparams=C.string_at(romptr+0x8b629,8*253) # $91:B629, original pose data
count=0;changed=0
for pose in [0x19,0x1a,0x1b,0x1c,0x4b,0x4c]:
 for prev in [2,3,20]:
  for speed,sub in [(0,0),(0,1),(1,0),(0xffff,1)]:
   for shooting in [0,512]:
    reset(128);setv('samus_pose',pose);setv('samus_x_base_speed',speed);setv('samus_x_base_subspeed',sub);setv('joypad1_lastkeys',shooting)
    C.memmove(ram+offsets['samus_prev_movement_type2'],bytes([prev]),1)
    before=read(0,131072);original();expected=getv('samus_new_pose')
    # Direction byte is at offset 3 in the original 8-byte pose record.
    eligible=prev in (3,20) or poseparams[pose*8+3]!=255
    if eligible and (speed or sub) and 0xa4<=expected<=0xa7:expected=10 if expected&1 else 9;changed+=1
    C.memmove(ram,before,len(before));landing();assert getv('samus_new_pose')==expected,(pose,prev,speed,sub,expected,getv('samus_new_pose'))
    count+=1
assert changed
results['landing']=dict(cases=count,changed=changed)
# Actual charged projectile construction: baseline damage versus the patch,
# and restoration when the real Charge upgrade is equipped.
damage=[]
for beams in [0,1,2,4,8,0xb]:
 values=[]
 for flags,charge in [(0,0),(256,0),(256,0x1000)]:
  reset(flags);setv('equipped_beams',beams|charge);setv('samus_pose',1)
  for n in ['projectile_counter','cooldown_timer','flare_counter']:setv(n,0)
  C.memset(ram+offsets['projectile_damage'],0,20)
  fire();values.append(getv('projectile_damage'))
 assert values[0]>0 and values[1]==values[0]//3 and values[2]==values[0],(beams,values)
 damage.append([beams,values])
results['chargedDamage']=damage
handler=routine('HudSelectionHandler_NothingOrPowerBombs',None)
for flags,charge in [(0,0),(256,0),(256,0x1000)]:
 reset(flags);setv('equipped_beams',charge);setv('hyper_beam_flag',0);setv('flare_counter',10)
 setv('new_projectile_direction_changed_pose',0);setv('joypad1_lastkeys',getv('button_config_shoot_x'))
 handler();assert getv('flare_counter')==(11 if flags or charge else 10)
results['chargeWithoutUpgrade']=True
# Use an original ROM enemy header and its vulnerability table, with enough
# health to avoid death side effects; verify actual contact damage ratio.
touch=routine('NormalEnemyTouchAiSkipDeathAnim',None)
contact=[]
for flags,charge in [(0,0),(256,0),(256,0x1000)]:
 reset(flags);setv('equipped_beams',charge);setv('cur_enemy_index',0);setv('samus_contact_damage_index',5)
 put(0xf78,0xdcff);put(0xf8c,30000)
 touch();contact.append(30000-word(0xf8c))
assert contact[0]>0 and contact[0]==contact[2] and contact[0]*33==contact[1]*100,contact
results['pseudoScrewDamage']=contact
costs=[]
for flags,charge,ammo,wanted in [(256,0,2,2),(256,0,3,0),(256,0,5,2),(256,0x1000,2,1),(0,0x1000,2,1)]:
 reset(flags);setv('equipped_beams',1|charge);setv('samus_power_bombs',ammo);setv('hud_item_index',3);setv('projectile_counter',0)
 result=sba();assert getv('samus_power_bombs')==wanted
 assert bool(result)==(ammo>=3 or charge!=0),(flags,charge,ammo,result)
 costs.append([flags,charge,ammo,wanted,getv('projectile_damage')])
assert costs[1][-1]==costs[3][-1] # SBA damage stays original, only cost changes.
results['sba']=costs
assert select(None,0,None);l.sm_seed_rules_configure(511);assert l.sm_seed_rules_active()==0
assert l.sm_cpu_opcodes()==0;l.sm_shutdown()
results.update(nativeSha256=hashlib.sha256(libpath.read_bytes()).hexdigest(),cpu=0,scope='Original native routines after boot; controlled states, not a manual playthrough')
(root/'gameplay/native-results.json').write_text(json.dumps(results,indent=2)+'\n')
print('NATIVE_GAMEPLAY_PATCHES_PASS',flush=True)
