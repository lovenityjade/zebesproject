#!/usr/bin/env python3
"""Native translations of reviewed Mirror room/enemy changes, upstream intact."""
from pathlib import Path
import sys,re
out=Path(sys.argv[1])
def edits(bank,changes):
 p=out/f'sm_{bank}_native.c';text=p.read_text()
 for function,old,new,count in changes:
  match=re.search(r'^\w+ '+re.escape(function)+r'\([^\n]*\) \{',text,re.M)
  assert match,function
  start=match.start()
  end=text.index('\n}',start)+2
  body=text[start:end]
  assert body.count(old)==count,(function,old,body.count(old))
  text=text[:start]+body.replace(old,new)+text[end:]
 p.write_text(text)
# boulders.asm: direction 2 retains its identity but tests Samus on the right,
# including initialization of the signed horizontal trigger distance.
edits('a6',[
 ('Boulder_Init','if (!boulder_parameter_1_high)',
  'if (!boulder_parameter_1_high || (sm_mirror_active() && boulder_parameter_1_high==2))',1),
 ('Boulder_Func_1','if (E->boulder_var_E)',
  'if (E->boulder_var_E && !(sm_mirror_active() && E->boulder_var_E==2))',1),
])
changes=[
 ('Etecoon_Func_9','__PAIR32__(32, 0)','__PAIR32__(sm_mirror_active()?0xffe0:32, 0)',1),
 ('Etecoon_Func_11','= addr_kEtecoon_Ilist_E894;',
  '= sm_mirror_active()?addr_kEtecoon_Ilist_E83C:addr_kEtecoon_Ilist_E894;',1),
 ('Etecoon_Func_11','= addr_kEtecoon_Ilist_E83C;',
  '= sm_mirror_active()?addr_kEtecoon_Ilist_E894:addr_kEtecoon_Ilist_E83C;',1),
]
for number,original,target,negative in [(16,537,487,True),(17,600,424,False),(18,600,424,False),(19,680,344,False),(20,840,184,False),(25,600,424,False)]:
 old=('' if negative else '!')+f'sign16(E->base.x_pos - {original})'
 new=('!' if negative else '')+f'sign16(E->base.x_pos - {target})'
 changes.append((f'Etecoon_Func_{number}',old,f'(sm_mirror_active()?{new}:{old})',1))
edits('a7',changes)
p=out/'sm_a7_native.c';text=p.read_text()
# These two ROM words have literal copies in the decompilation. Read the actual
# data words in Mirror so run/rebound/tunnel paths share the source velocities.
for address,count in [('E908',6),('E90C',4)]:
 old=f'= g_word_A7{address};';new=f'= sm_mirror_active()?GET_WORD(RomPtr_A7(0x{address})):g_word_A7{address};'
 assert text.count(old)==count,(address,text.count(old));text=text.replace(old,new)
p.write_text(text)
edits('84',[
 ('PlmPreInstr_WakeAndLavaIfBoosterCollected','fx_y_vel = -128;','fx_y_vel = sm_mirror_active()?-255:-128;',1),
 ('PlmPreInstr_WakePLMAndStartFxMotionSamusFarLeft','samus_x_pos <= 0xAE0','samus_x_pos <= (sm_mirror_active()?0:0xAE0)',1),
 ('PlmPreInstr_AdvanceLavaSamusMovesLeft','  int v1 = k >> 1;',
  '  const uint16 *lava_data=sm_mirror_active()?(const uint16*)RomPtr_84(0xb876):g_word_84B876;\n  int v1 = k >> 1;',1),
 ('PlmPreInstr_AdvanceLavaSamusMovesLeft','g_word_84B876[v3','lava_data[v3',4),
 ('PlmPreInstr_DeletePlmAndSpawnTriggerIfBlockDestroyed','prod + 4','prod + (sm_mirror_active()?43:4)',1),
 ('PlmSetup_D6DA_LowerNorfairChozoHandTrigger','{ 0x0c, 0x1d, 0xd113 }','{ sm_mirror_active()?0x23:0x0c, 0x1d, 0xd113 }',1),
 ('PlmSetup_D6F2_WreckedShipChozoHandTrigger','&scrolls[7]','&scrolls[sm_mirror_active()?9:7]',1),
 ('PlmSetup_D6F2_WreckedShipChozoHandTrigger','&scrolls[13]','&scrolls[sm_mirror_active()?15:13]',1),
 ('PlmSetup_D6F2_WreckedShipChozoHandTrigger','{ 0x17, 0x1d, 0xd6f8 }','{ sm_mirror_active()?0x3a:0x17, 0x1d, 0xd6f8 }',1),
])
