"""Boss Rush native hooks, applied to generated banks only. Fail on drift."""
from pathlib import Path
import sys
src,out=map(Path,sys.argv[1:])
def patch(bank,replacements):
 p=out/('sm_90_movement.c' if bank=='90' else f'sm_{bank}_native.c')
 s=p.read_text() if p.exists() and bank not in ('94','a4') else (src/f'sm_{bank}.c').read_text()
 for old,new in replacements:
  assert s.count(old)==1,(bank,old,s.count(old))
  s=s.replace(old,new)
 p.write_text('#include "sm_rush_runtime.h"\n'+s)
patch('80',[('  sm_test_load_room();','  sm_test_load_room();\n  sm_rush_load_room();')])
patch('81',[('void SaveToSram(uint16 a) {','void SaveToSram(uint16 a) {\n  if(sm_rush_active())return;')])
patch('90',[
 ('  t = sm_relic_escape_periodic_damage(t);','  t = sm_rush_active()?sm_rush_periodic(t):sm_relic_escape_periodic_damage(t);'),
 ('void DrawSamusAndProjectiles(void) {','void DrawSamusAndProjectiles(void) {\n  sm_rush_draw_pose();'),
])
patch('91',[
 ('      a = sm_relic_escape_damage(a);','      a = sm_rush_active()?sm_rush_damage(a):sm_relic_escape_damage(a);'),
 ('    if (a != 300 && !time_is_frozen_flag) {','    if ((a != 300 || sm_rush_active()) && !time_is_frozen_flag) {'),
])
patch('94',[(f'uint8 BlockColl_{axis}_Door(CollInfo *ci) {{',f'uint8 BlockColl_{axis}_Door(CollInfo *ci) {{\n  if(sm_rush_block_doors())return BlockColl_{axis}_SolidShootGrappleBlock(ci);') for axis in ('Horiz','Vert')])
patch('86',[
 ('uint16 RandomDropRoutine(uint16 k) {','static uint16 sm_rush_original_drop(uint16 k) {'),
 ('  Samus_RestoreHealth(5);','  Samus_RestoreHealth(sm_rush_pickup(5));'),
 ('  Samus_RestoreHealth(0x14);','  Samus_RestoreHealth(sm_rush_pickup(0x14));'),
 ('  Samus_RestorePowerBombs(1);','  Samus_RestorePowerBombs(sm_rush_pickup(1));'),
 ('  Samus_RestoreMissiles(2);','  Samus_RestoreMissiles(sm_rush_pickup(2));'),
 ('  Samus_RestoreSuperMissiles(1);','  Samus_RestoreSuperMissiles(sm_rush_pickup(1));'),
])
with (out/'sm_86_native.c').open('a') as f:f.write('\nuint16 RandomDropRoutine(uint16 k){return sm_rush_drop(sm_rush_original_drop(k));}\n')
patch('a9',[
 ('void MotherBomb_FiringRainbowBeam_0(void) {','void MotherBomb_FiringRainbowBeam_0(void) {\n  sm_rush_mb_sequence(1);'),
 ('void MotherBrain_Phase3_Recover_SetupForFight(void) {','void MotherBrain_Phase3_Recover_SetupForFight(void) {\n  sm_rush_mb_sequence(0);'),
 ('void Samus_DamageDueToRainbowBeam(void) {','void Samus_DamageDueToRainbowBeam(void) {\n  if(sm_rush_mb_visual())return;'),
 ('  uint16 v0 = samus_health + (equipped_items & 1) - 2;', '  uint16 drain=2-(equipped_items&1);\n  uint16 v0 = samus_health - (sm_rush_active()?sm_rush_damage(drain):drain);'),
 ('    if (sign16(samus_health - 400)) {','    if (sm_rush_mb_visual() || sign16(samus_health - 400)) {'),
 ('  if ((int16)(4 * v0 + 20 - samus_health) >= 0) {','  if (sm_rush_mb_visual() || (int16)(4 * v0 + 20 - samus_health) >= 0) {'),
 ('void MotherBrain_Phase3_Death_15_LoadEscapeTimerTiles(void) {','void MotherBrain_Phase3_Death_15_LoadEscapeTimerTiles(void) {\n  if(sm_rush_mb_finish())return;'),
])

# Off-screen fatal hits can leave Draygon at a wrapped negative coordinate.
# Use signed room coordinates for the death approach, scoped to Rush.
patch('a5',[(
 'CalculateAngleFromXY(64 - (E->base.x_pos >> 2), 120 - (E->base.y_pos >> 2))',
 'CalculateAngleFromXY(64 - ((sm_rush_active()?(int16)E->base.x_pos:E->base.x_pos) >> 2), 120 - ((sm_rush_active()?(int16)E->base.y_pos:E->base.y_pos) >> 2))'
),(
 '  enemy_bg2_tilemap_size = 1024;',
 '  enemy_bg2_tilemap_size = sm_rush_active()?4096:1024;'
),(
 '  uint16 v6 = varE26;',
 '  if(sm_rush_active())REMOVED_varE26=varE26; /* A5:8889 / A5:89C9 shared scratch. */\n  uint16 v6 = varE26;'
),(
 '  printf("Wtf is varE26?\\n");\n  uint16 varE26 = 0; // Wtf is varE26',
 '  uint16 varE26 = sm_rush_active()?REMOVED_varE26:0;'
),(
 'if ((int32)E->base.x_pos < 0 && sign16(E->base.x_pos + 80))',
 'if ((sm_rush_active()?(int16)E->base.x_pos:(int32)E->base.x_pos) < 0 && sign16(E->base.x_pos + 80))'
)])

# Scale only the living Crocomire recoil, never his advance or death movement.
patch('a4',[
 ('Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, INT16_SHL16(4));',
  'Enemy_MoveRight_IgnoreSlopes(cur_enemy_index, sm_rush_crocomire_recoil(INT16_SHL16(4)));'),
 ('Enemy_MoveRight_IgnoreSlopes(k, INT16_SHL16(4));',
  'Enemy_MoveRight_IgnoreSlopes(k, sm_rush_crocomire_recoil(INT16_SHL16(4)));'),
])

# Rush restores a menu snapshot before loading each arena. Phantoon uploads
# only 864 bytes normally, leaving the rest of BG2 filled with menu tile 0.
# Tile 0 is a wall in his tileset and follows his body scroll. His initializer
# already clears all 4096 bytes to transparent tile 0x338: upload that full map.
patch('a7', [('  enemy_bg2_tilemap_size = 864;',
              '  enemy_bg2_tilemap_size = sm_rush_active()?4096:864;')])
