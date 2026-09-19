"""Final battle presentation hooks, all modes; assert unique upstream anchors."""
from pathlib import Path
import sys
out=Path(sys.argv[1])
def patch(name, pairs):
 p=out/name;s=p.read_text()
 for old,new in pairs:
  assert s.count(old)==1,(name,old,s.count(old))
  s=s.replace(old,new)
 p.write_text('#include "sm_finale.h"\n'+s)
def phase(function, n, args='void'):
 old=f'void {function}({args}) {{'
 return old,old+f'\n  sm_finale_phase({n});'
pairs=[
 phase('ShitroidInCutscene_ActivateRainbowBeam',1,'uint16 k'),
 phase('ShitroidInCutscene_MoveUpToCeiling',2,'uint16 k'),
 phase('ShitroidInCutscene_InitiateFinalCharge',3,'uint16 k'),
 phase('ShitroidInCutscene_ShitroidFinalBelow',4,'uint16 k'),
 phase('ShitroidInCutscene_DeathSequence',5,'uint16 k'),
 phase('MotherBrain_Phase3_Recover_SetupForFight',6),
 phase('MotherBrain_Phase3_Death_0',7),
 phase('MotherBrain_Phase3_Death_14_20framedelay',8),
 ('void ShitroidInCutscene_Main(void) {','void ShitroidInCutscene_Main(void) {\n  sm_finale_baby(cur_enemy_index);'),
 ('void MotherBrain_Phase3_BeamShotReaction(void) {','void MotherBrain_Phase3_BeamShotReaction(void) {\n  sm_finale_hit();'),
 ('    ShitroidInCutscene_HandleShitroidDeathExplosions(k);','    if(!sm_finale_enabled())ShitroidInCutscene_HandleShitroidDeathExplosions(k);'),
]
patch('sm_a9_native.c',pairs)
patch('sm_90_movement.c',[(
 'void SpawnProjectileTrail(uint16 k) {',
 'void SpawnProjectileTrail(uint16 k) {\n  if(sm_finale_hyper_enabled() && (projectile_type[k >> 1] & 0xfff)==0x18)return;'),(
 '      projectile_bomb_pre_instructions[v3] = FUNC16(ProjPreInstr_HyperBeam);',
 '      projectile_bomb_pre_instructions[v3] = FUNC16(ProjPreInstr_HyperBeam);\n      sm_finale_shot(v3);')])

# Replace just the Hyper projectile art; native movement/collisions and
# damage remain unchanged. Missing textures retain the original sprite.
src=Path(__file__).resolve().parent.parent/'native-core/src/sm_93.c'
s=src.read_text()
old='DrawProjectileSpritemap(v0, r20, r18);'
assert s.count(old)==3
s=s.replace(old,'if (!(sm_finale_hyper_enabled() && (projectile_type[v0 >> 1] & 0xfff) == 0x18)) '+old)
(out/'sm_93_native.c').write_text('#include "sm_finale.h"\n'+s)
