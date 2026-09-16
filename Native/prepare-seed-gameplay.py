#!/usr/bin/env python3
"""Exact movement/charge seams; the upstream translation stays pristine."""
import sys
from pathlib import Path
p=Path(sys.argv[1]);s=p.read_text()
def replace(old,new,count=1):
 global s
 assert s.count(old)==count,(old,s.count(old));s=s.replace(old,new)
replace('#include "sm_connections.h"','#include "sm_seed_rules.h"\n#include "sm_connections.h"')
replace('#include "sm_seed_rules.h"','#include "sm_relic.h"\n#include "sm_seed_rules.h"')
replace('#include "sm_relic.h"','#include "sm_item_selection.h"\n#include "sm_relic.h"')
replace('void HandleSwitchingHudSelection(void) {',
        'void HandleSwitchingHudSelection(void) {\n  int direction = sm_item_selection_direction();')
replace('  if ((button_config_itemswitch & joypad1_newkeys) == 0)\n    goto LABEL_13;\n  v0 = hud_item_index + 1;\n  if (!sign16(hud_item_index - 5))\n    goto LABEL_5;',
        '  if (!direction && (button_config_itemswitch & joypad1_newkeys) == 0)\n    goto LABEL_13;\n  if(!direction)direction=1;\n  v0 = (hud_item_index + direction + 6) % 6;')
replace('    v0 = hud_item_index + 1;\n    hud_item_index = v0;\n    if (!sign16(v0 - 6)) {\n      v0 = 0;\n      hud_item_index = 0;\n    }',
        '    v0 = (hud_item_index + direction + 6) % 6;\n    hud_item_index = v0;')
replace('  AddToHiLo(&samus_health, &samus_subunit_health, -t);',
        '  t = sm_relic_escape_periodic_damage(t);\n  AddToHiLo(&samus_health, &samus_subunit_health, -t);')
for ammo in ['samus_missiles','samus_super_missiles','samus_power_bombs']:
 replace('|| sign16('+ammo+' - 10)','|| sign16('+ammo+' - (sm_seed_rule(SM_SEED_ROUND_ROBIN_CF)?0:10))')
replace('  which_item_to_pickup = 0;\n  substate = 10;','  which_item_to_pickup = 0;\n  substate = sm_seed_rule(SM_SEED_ROUND_ROBIN_CF)?30:10;')
replace('void SamusMoveHandler_CrystalFlashMain(void) {','void SamusMoveHandler_CrystalFlashMain(void) {\n  if(sm_seed_crystal_flash())return;')
replace('hyper_beam_flag || (equipped_beams & 0x1000) == 0','hyper_beam_flag || ((equipped_beams & 0x1000) == 0 && !sm_seed_rule(SM_SEED_NERFED_CHARGE))')
replace('if ((equipped_beams & 0x1000) != 0\n          ||','if ((equipped_beams & 0x1000) != 0 || sm_seed_rule(SM_SEED_NERFED_CHARGE)\n          ||')
# Limit the damage hook to the charged-beam routine, before collision checks.
start=s.index('void FireChargedBeam(void)');end=s.index('static const int16 kProjectileOriginOffsets3_X',start)
part=s[start:end];assert part.count('InitializeProjectile(')==1
line=next(line for line in part.splitlines() if 'InitializeProjectile(' in line)
arg=line.split('InitializeProjectile(')[1].split(')')[0]
part=part.replace(line,line+'\n      if(sm_seed_rule(SM_SEED_NERFED_CHARGE) && !(equipped_beams&0x1000))projectile_damage[('+arg+')>>1]/=3;')
s=s[:start]+part+s[end:]
replace('  v2 = samus_power_bombs - kCostOfSbaInPowerBombs[v1 >> 1];','''  int cost=kCostOfSbaInPowerBombs[v1 >> 1];
  if(sm_seed_rule(SM_SEED_NERFED_CHARGE) && !(equipped_beams&0x1000)){
    cost*=3;if((int16)(samus_power_bombs-cost)<0)return 0;
  }
  v2 = samus_power_bombs - cost;''')
replace('  if ((equipped_items & 0x20) != 0)\n    t = (t >> 2) & 0xffff00;',
    '  if(sm_seed_rule(SM_SEED_BALANCED_SUITS|SM_SEED_PROGRESSIVE_SUITS))\n    t = sm_seed_periodic_damage(t);\n  else if ((equipped_items & 0x20) != 0)\n    t = (t >> 2) & 0xffff00;')
# The source BRA at $90:A493 skips both speed windows, retaining the
# descent, equipment, liquid and distinct-press conditions around them.
start=s.index('void Samus_Movement_03_SpinJumping(void)');end=s.index('void Samus_Movement_04_',start)
part=s[start:end];a=part.index('      if (liquid_physics_type) {');b=part.index('      UNUSED_word_7E0DFA',a)
part=part[:a]+'      if(!sm_seed_rule(SM_SEED_INFINITE_SPACEJUMP)){\n'+part[a:b]+'      }\n'+part[b:]
s=s[:start]+part+s[end:]
p.write_text(s)
