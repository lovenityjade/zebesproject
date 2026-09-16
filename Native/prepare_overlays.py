#!/usr/bin/env python3
"""Generate auditable native adaptations while leaving upstream pristine."""
from pathlib import Path
import sys
source=Path(sys.argv[1]);out=Path(sys.argv[2])

def write(bank,replacements):
    text='#include "sm_runs.h"\n#include "sm_soundtrack.h"\n#include "sm_animals.h"\n#include "sm_mirror.h"\n#include "sm_escape.h"\n#include "sm_minimizer.h"\n#include "sm_tourian.h"\n#include "sm_scavenger.h"\n#include "sm_objective_pause.h"\n#include "sm_objectives.h"\n#include "sm_objective_events.h"\n#include "sm_areas.h"\n#include "sm_doors.h"\n#include "sm_connections.h"\n#include "sm_world_data.h"\n'+(source/f'sm_{bank}.c').read_text()
    for old,new,count in replacements:
        actual=text.count(old)
        if actual!=count:raise RuntimeError(f'{bank}: expected {count} matches, got {actual}: {old[:100]}')
        text=text.replace(old,new)
    (out/f'sm_{bank}_native.c').write_text('#include "sm_start.h"\n#include "sm_seed_rules.h"\n#include "sm_save_refill.h"\n#include "sm_relic.h"\n#include "sm_travel.h"\n#include "sm_map_browser.h"\n#include "sm_cinematics.h"\n#include "sm_credits.h"\n#include "sm_run_stats.h"\n#include "sm_varia_ui.h"\n#include "sm_generation.h"\n#include "sm_seed.h"\n#include "sm_effects.h"\n#include "sm_sprite_view.h"\n#include "sm_route.h"\n'+text)

write('80', [
    ('  RtlApuWrite(APUI00, 0);','  RtlApuWrite(APUI00, sm_soundtrack_command(music_data_index, 0));',1),
    ('    RtlApuWrite(APUI00, music_entry & 0x7F);','    RtlApuWrite(APUI00, sm_soundtrack_command(music_data_index, music_entry & 0x7F));',1),
    ('  SetTimerMinutes(0x300);','  SetTimerMinutes(sm_escape_timer());',1),
    ('  if (kDoorTransitionFuncs[door_direction & 3]()) {',
     '  if (kDoorTransitionFuncs[door_direction & 3]() || (sm_seed_rule(SM_SEED_FAST_DOORS) && kDoorTransitionFuncs[door_direction & 3]())) {',1),
    ('  RtlWriteSram();','  sm_save();',2),
    ('  LOBYTE(debug_disable_minimap) = 0;', '  LOBYTE(debug_disable_minimap) = 0;\n  sm_test_load_room();',1),
    ('void ClearOamExt(void) {', 'void ClearOamExt(void) {\n  sm_sprite_view_reset();', 1),
    ('  DrawSpritemap(0x80, j, a + HIBYTE(timer_x_pos), HIBYTE(timer_y_pos), 2560);',
     '  sm_sprite_view_set_gui(1);\n  DrawSpritemap(0x80, j, a + HIBYTE(timer_x_pos), HIBYTE(timer_y_pos), 2560);\n  sm_sprite_view_set_gui(0);',1),
])
# Match each complete routine, keeping upstream's OAM writes unchanged.
sprite_source=(source/'sm_81.c').read_text()
for name,base in [('DrawSpritemap','x_r20'),('DrawSpritemapOffScreen','x_r20'),
                  ('DrawMenuSpritemap','k'),('DrawSamusSpritemap','x_pos'),
                  ('DrawGrappleOrProjectileSpritemap','x_r20'),
                  ('DrawSpritemapWithBaseTile','r20_x'),('DrawSpritemapWithBaseTile2','r20_x'),
                  ('DrawSpritemapWithBaseTileOffscreen','r20_x'),
                  ('DrawEprojSpritemapWithBaseTile','x_r20'),('DrawEprojSpritemapWithBaseTileOffscreen','x_r20')]:
    start=sprite_source.index('void '+name+'(');end=sprite_source.index('\n}\n',start)+3
    body=sprite_source[start:end]
    # All these routines advance pp after writing the complete OAM entry.
    assert body.count('    pp += 5;')==1
    # Some routines advance idx first; obtain the entry they just wrote.
    previous='idx = (idx + 4) & 0x1FF;' in body.split('    pp += 5;')[0] or 'idx += 4;' in body.split('    pp += 5;')[0]
    index='(idx-4)&0x1ff' if previous else 'idx'
    base_y='j' if name=='DrawMenuSpritemap' else 'y_pos' if name=='DrawSamusSpritemap' else 'r18_y' if base=='r20_x' else 'y_r18'
    body=body.replace('    pp += 5;',f'    sm_sprite_view_record({index},{base},GET_WORD(pp),{base_y},pp[2]);\n    pp += 5;')
    sprite_source=sprite_source[:start]+body+sprite_source[end:]
assert sprite_source.count('RtlWriteSram();')==4
sprite_source=sprite_source.replace('RtlWriteSram();','sm_save();')
sprite_source=sprite_source.replace('void SaveToSram(uint16 a) {','void SaveToSram(uint16 a) {\n  sm_relic_escape_checkpoint(a);',1)
for old,new in [('  PackMapToSave();','  PackMapToSave();\n  sm_map_exploration_pack();'),('    UnpackMapFromSave();','    UnpackMapFromSave();\n    sm_map_exploration_unpack();')]:
    assert sprite_source.count(old)==1,old
    sprite_source=sprite_source.replace(old,new)
sprite_source='#include \"sm_map_exploration.h\"\n'+sprite_source
assert sprite_source.count('uint8 LoadFromSram(uint16 a) {')==1
sprite_source=sprite_source.replace('uint8 LoadFromSram(uint16 a) {','uint8 LoadFromSram(uint16 a) {\n  sm_tracker_new_session();')
for old,new in [
    ('void DrawFileSelectSlotSamusHelmet(uint16 k) {','void DrawFileSelectSlotSamusHelmet(uint16 k) {\n  if(sm_slots_mode_badges_active())return;'),
    ('void FileSelectMap_6_AreaSelectMap(void) {','void FileSelectMap_6_AreaSelectMap(void) {\n  if(sm_travel_area_input())return;'),
    ('void FileSelectMap_10_RoomSelectMap(void) {','void FileSelectMap_10_RoomSelectMap(void) {\n  if(sm_travel_room_input())return;'),
    ('    r3 = (i == file_select_map_area_index) ? 0 : 512;', '    r3 = (i == file_select_map_area_index) ? 0 : 512;\n    if(sm_map_browser_overview() || (game_state==5 && sm_travel_enabled())){\n      int area=kFileSelectMap_AreaIndexes[i];\n      if(sm_map_browser_overview()?sm_map_browser_area_visible(area):sm_travel_station_mask(area))DrawMenuSpritemap(g_word_82C749[0]+area+1,kAreaSelectMapLabelPositions[area*2],kAreaSelectMapLabelPositions[area*2+1],r3);\n      continue;\n    }'),
    ('  v0 = selected_save_slot;\n  if (sign16(selected_save_slot - 3)) {','  v0 = selected_save_slot;\n  if (sign16(selected_save_slot - 3)) {\n    if (!sm_slots_select(selected_save_slot)) {QueueSfx1_Max6(0x3d);return;}'),
    ('void QueueTransferOfMenuTilemapToVramBG1(void) {','void QueueTransferOfMenuTilemapToVramBG1(void) {\n  if (game_state==4 && (menu_index==2 || menu_index==16)) sm_slots_badges();'),
    ('void FileSelectMenu_13_FileCopyDoIt(void) {','void FileSelectMenu_13_FileCopyDoIt(void) {\n  if (!sm_slots_action(2,eproj_id[16],eproj_id[17])) {menu_index=12;QueueSfx1_Max6(0x3d);return;}'),
    ('void FileSelectMenu_25_FileClearDoClear(void) {','void FileSelectMenu_25_FileClearDoClear(void) {\n  if (!sm_slots_action(3,eproj_id[16],0)) {menu_index=24;QueueSfx1_Max6(0x3d);return;}'),
]:
    assert sprite_source.count(old)==1,old
    sprite_source=sprite_source.replace(old,new)
(out/'sm_81_native.c').write_text('#include "sm_start.h"\n#include "sm_seed_rules.h"\n#include "sm_save_refill.h"\n#include "sm_relic.h"\n#include "sm_travel.h"\n#include "sm_map_browser.h"\n#include "sm_generation.h"\n#include "sm_tracker.h"\n#include "sm_bridge.h"\n#include "sm_sprite_view.h"\n#include "sm_route.h"\n'+sprite_source)

write('a8',[
    ('(collected_items & 4) != 0','sm_seed_morph_collected()',2),
    ('(collected_items & 4) == 0','!sm_seed_morph_collected()',1),
])
write('8f',[
    # The original Zebes escape states replace Crateria exits with sealed gray
    # doors. Tablet escape starts anywhere: retain the normal room topology.
    # Keep event 14 itself for the timer/ending, and keep every other condition.
    ('  if (CheckEventHappened(*v1))',
     '  if (CheckEventHappened(*v1) && !(*v1==14 && sm_relic_required()>0 && sm_relic_count()>=sm_relic_required()))',1),
    ('void CallRoomSetupCode(uint32 ea) {','void CallRoomSetupCode(uint32 ea) {\n  if(sm_animals_room(ea))return;',1),
    ('0x3d, 0x0b, 0xbb30', 'sm_mirror_active()?0x0d:0x3d, 0x0b, 0xbb30', 1),
    ('0x10, 0x87, 0xb964', 'sm_mirror_active()?0x1e:0x10, 0x87, 0xb964', 1),
    ('0x0f, 0x0a, 0xb9ed', 'sm_mirror_active()?0:0x0f, 0x0a, 0xb9ed', 1),
    ('0x04, 0x09, 0xb64b', '0x04, 0x09, sm_mirror_active()?0xb64f:0xb64b', 1),
    ('0x04, 0x09, 0xb64f', '0x04, 0x09, sm_mirror_active()?0xb64b:0xb64f', 1),
    ('void RunRoomSetupCode(void) {  // 0x8FE88F','void RunRoomSetupCode(void) {  // 0x8FE88F\n  sm_escape_room_setup();',1),
    ('void RunRoomMainCode(void) {  // 0x8FE8BD','void RunRoomMainCode(void) {  // 0x8FE8BD\n  sm_escape_room_main();',1),
    ('void RunDoorSetupCode(void) {  // 0x8FE8A3','void RunDoorSetupCode(void) {  // 0x8FE8A3\n  if(sm_escape_door() || sm_tourian_door() || sm_minimizer_door() || sm_connections_door() || sm_areas_door())return;',1),
    ('void CallDoorDefSetupCode(uint32 ea) {','void CallDoorDefSetupCode(uint32 ea) {\n  if(sm_animals_door(ea) || sm_mirror_door(ea))return;\n  if(ea==0x8ff701){sm_areas_refill();return;}',1),
    ('if ((collected_items & 4) == 0 || !samus_max_missiles)',
     'if (sm_seed_active() ? !(events_that_happened[0]&1) : ((collected_items & 4) == 0 || !samus_max_missiles))',1),
])
write('82',[
    ('void PlayRoomMusicTrackAfterAFrames(uint16 a) {',
     'void PlayRoomMusicTrackAfterAFrames(uint16 a) {\n  if(sm_seed_rule(SM_SEED_ITEM_SOUNDS) && game_state<40){uint16 kind=debug_saved_yscroll;debug_saved_yscroll=0;if(kind==2)return;}',1),

    ('  room_main_code_ptr = RD->main_code_ptr;','  room_main_code_ptr = RD->main_code_ptr;\n  sm_escape_load_header();',1),
    ('      if ((layer1_x_pos & 0x80) != 0)\n        ++layer1_x_pos;',
     '      if (sm_seed_rule(SM_SEED_FAST_DOORS)) sm_seed_align_camera(&layer1_x_pos);\n      else if ((layer1_x_pos & 0x80) != 0)\n        ++layer1_x_pos;',1),
    ('    if ((layer1_y_pos & 0x80) != 0)\n      ++layer1_y_pos;',
     '    if (sm_seed_rule(SM_SEED_FAST_DOORS)) sm_seed_align_camera(&layer1_y_pos);\n    else if ((layer1_y_pos & 0x80) != 0)\n      ++layer1_y_pos;',1),

    ('    cinematic_function = FUNC16(CinematicFunctionEscapeFromCebes);',
     '    cinematic_function = sm_relic_ending()?FUNC16(CinematicFunction_Intro_Func126):FUNC16(CinematicFunctionEscapeFromCebes);',1),
    ('void DrawFileSelectMapIcons(void) {', 'void DrawFileSelectMapIcons(void) {\n  sm_travel_draw_stations();',1),
    ('  DrawBossMapIcons(9, addr_kMapIconDataPointers);','  if(!sm_objectives_state(0)) DrawBossMapIcons(9, addr_kMapIconDataPointers);',1),
    ('  ReleaseButtonsFilter(3);\n  MainPauseRoutine();','  ReleaseButtonsFilter(3);\n  if(sm_map_browser_tick())return kCoroutineNone;\n  MainPauseRoutine();',1),
    ('void MainPauseRoutine(void) {','void MainPauseRoutine(void) {\n  if(sm_objective_pause_tick())return;',1),
    ('  LoadEquipmentScreenEquipmentTilemaps();\n  SetPauseScreenButtonLabelPalettes_0();','  LoadEquipmentScreenEquipmentTilemaps();\n  SetPauseScreenButtonLabelPalettes();',1),
    ('void HandlePauseScreenLrInput(void) {','void HandlePauseScreenLrInput(void) {\n  if(sm_objective_pause_input())return;',1),
    ('void DrawLrHighlight(void) {','void DrawLrHighlight(void) {\n  if(sm_objective_pause_highlights())return;',1),
    ('void SetPauseScreenButtonLabelPalettes(void) {','void SetPauseScreenButtonLabelPalettes(void) {\n  if(sm_objective_pause_buttons())return;',1),
    ('void DrawPauseMenuDuringFadeout(void) {','void DrawPauseMenuDuringFadeout(void) {\n  if(pause_screen_mode==2){HandlePauseMenuLRPressHighlight();return;}',1),
    ('  ClearSamusBeamTiles();\n  ContinueInitGameplayResume();','  sm_objective_pause_unpause();\n  ClearSamusBeamTiles();\n  ContinueInitGameplayResume();',1),
    ('LABEL_10:\n    CalculateLayer2PosAndScrollsWhenScrolling();','LABEL_10:\n    sm_stats_add(5);\n    CalculateLayer2PosAndScrollsWhenScrolling();',1),
    ('uint8 RefillHealthFromReserveTanks(void) {', 'uint8 RefillHealthFromReserveTanks(void) {\n  if(sm_varia_ui_active(SM_VARIA_RESERVES))return sm_varia_reserve_transfer();',1),
    ('void HandleSamusOutOfHealthAndGameTile(void) {', 'void HandleSamusOutOfHealthAndGameTile(void) {\n  if(sm_varia_ui_active(SM_VARIA_RESERVES) && (int16)samus_health<=0 && game_state!=8)return;',1),
    ('void EquipmentScreenCategory_Tanks_1(void) {', 'void EquipmentScreenCategory_Tanks_1(void) {\n  if(sm_varia_reserve_manual())return;',1),
    ('void EquipmentScreenMain(void) {', 'void EquipmentScreenMain(void) {\n  if(sm_varia_ui_active(SM_VARIA_RESERVES) && pausemenu_equipment_category_item!=0x100)pausemenu_reserve_tank_delay_ctr=0;',1),

    ('const uint16 *r6 = (const uint16 *)RomPtr_82(kPauseMenuMapData[v0]);',
     'const uint16 *r6 = (const uint16 *)sm_seed_map_data(v0);',2),
    ('void DisplayMapElevatorDestinations(void) {  // 0x82BB30\n  if (map_station_byte_array[area_index]) {',
     'void DisplayMapElevatorDestinations(void) {  // 0x82BB30\n  if ((sm_map_fully_known() && area_index<6) || map_station_byte_array[area_index]) {',1),
    ('if (map_station_byte_array[area_index]) {','if (sm_map_fully_known() || map_station_byte_array[area_index]) {',2),
    ('if (has_area_map) {','if (sm_map_fully_known() || has_area_map) {',2),
    ('r6 = RomPtr_82(GET_WORD(RomPtr_82(addr_kPauseMenuMapData + 2 * area_index)));',
     'r6 = sm_seed_map_data(area_index);',1),
    ('return (kBits0x80Shr[v3] & map_tiles_explored[r18]) != 0;',
     'return (kBits0x80Shr[v3] & (sm_map_fully_known() && r18<256 ? sm_seed_map_data(area_index)[r18] : map_tiles_explored[r18])) != 0;',1),
    ('  menu_option_index = 0;\n  DeleteAllOptionsMenuObjects_();', '  sm_generation_enter();\n  DeleteAllOptionsMenuObjects_();',1),
    ('void OptionsPreInstr_F2A9(uint16 v0) {', 'void OptionsPreInstr_F2A9(uint16 v0) {\n  if(sm_generation_cursor(v0))return;',1),
    ('void GameOptionsMenu_3_OptionsScreen(void) {',
     'void GameOptionsMenu_3_OptionsScreen(void) {\n  if (sm_generation_input()) return;',1),
    ('  OptionsMenuFunc1();\n  DrawOptionsMenuSpritemaps();',
     '  sm_generation_draw();\n  OptionsMenuFunc1();\n  DrawOptionsMenuSpritemaps();',1),
    ('void GameOptionsMenuItemFunc_0(void) {',
     'void GameOptionsMenuItemFunc_0(void) {\n  if (!sm_generation_can_start()) return;',1),
    ('  DrawPauseScreenSpriteAnim(v1[2], *v1, v1[1]);',
     '  sm_sprite_view_set_gui(1);\n  DrawPauseScreenSpriteAnim(v1[2], *v1, v1[1]);\n  sm_sprite_view_set_gui(0);',1),
    ('void GameOptionsMenu_4_StartGame(void) {',
     'void GameOptionsMenu_4_StartGame(void) {\n  if (!sm_generation_can_start()) {game_options_screen_index=3;return;}\n  if(!loading_game_state){sm_run_new_game();sm_route_new_game();}\n  if (sm_seed_active() && !loading_game_state) { sm_start_new_game();return; }',1),
    ('  RunDoorSetupCode();','  sm_seed_room_items();\n  sm_start_room();\n  sm_start_door();\n  RunDoorSetupCode();',2),
])
write('84',[
    ('    uint16 v3 = level_data[v2] & 0xF000 | 0xB6;', '    uint16 v3 = level_data[v2] & 0xF000 | 0xB6;\n    sm_visual_sprite((v2%room_width_in_blocks)*16+8,(v2/room_width_in_blocks)*16+8,144);',1),
    ('uint8 PlmSetup_BB30_CrateriaMainstreetEscape(uint16 j) {  // 0x84BB09\n  if (!CheckEventHappened(0xF))', 'uint8 PlmSetup_BB30_CrateriaMainstreetEscape(uint16 j) {  // 0x84BB09\n  if (!CheckEventHappened(sm_animals_escape_event()))',1),
    ('const uint8 *PlmInstr_ClearMusicQueueAndQueueTrack(const uint8 *plmp, uint16 k) {',
     'const uint8 *PlmInstr_ClearMusicQueueAndQueueTrack(const uint8 *plmp, uint16 k) {\n  if(sm_seed_pickup_sound(plmp))return plmp+1;\n  if(sm_seed_rule(SM_SEED_ITEM_SOUNDS)){debug_saved_yscroll=1;CancelSoundEffects();}',1),

    ('  *(uint16 *)&map_station_byte_array[area_index] |= 0xFF;','  if(sm_escape_enabled())memset(map_station_byte_array,255,8);\n  *(uint16 *)&map_station_byte_array[area_index] |= 0xFF;',1),
    ('  int r = DisplayMessageBox_Poll(23);','  if(sm_escape_active() && !sm_relic_escape_active())return INSTRB_RETURN_ADDR(GET_WORD(plmp));\n  int r = DisplayMessageBox_Poll(23);',1),
    ('  if ((projectile_type[projectile_index >> 1] & 0xFFF) == 512)',
     '  if ((projectile_type[projectile_index >> 1] & 0xFFF) == 512 || sm_escape_hyper(projectile_index >> 1))',2),
    ('  } else if (v1 == 768) {','  } else if (v1 == 768 || sm_escape_hyper(projectile_index >> 1)) {',2),
    ('  } else if (v1 == 512) {','  } else if (v1 == 512 || sm_escape_hyper(projectile_index >> 1)) {',1),

    ('const uint8 *PlmInstr_IncrementDoorHitCounterAndJGE(const uint8 *plmp, uint16 k) {',
     'const uint8 *PlmInstr_IncrementDoorHitCounterAndJGE(const uint8 *plmp, uint16 k) {\n  if(sm_tourian_fast() && room_ptr==0xddc4 && plmp==RomPtr_84(0xd889))return plmp+3;',1),

    ('const uint8 *PlmInstr_SetItemBit(const uint8 *plmp, uint16 k) {  // 0x848899',
     'const uint8 *PlmInstr_SetItemBit(const uint8 *plmp, uint16 k) {  // 0x848899\n  if(!sm_scavenger_pickup(plm_room_arguments[k >> 1]))return plmp-6;',1),
    ('void CallPlmPreInstr(uint32 ea, uint16 k) {', 'void CallPlmPreInstr(uint32 ea, uint16 k) {\n  if(sm_doors_preinstr(ea,k))return;',1),
    ('if (plm_header_ptr[v1] >= FUNC16(PlmPreInstr_GotoLinkIfTriggered))',
     'if (plm_header_ptr[v1] >= FUNC16(PlmPreInstr_GotoLinkIfTriggered) && !sm_indicators_is_plm(plm_header_ptr[v1]) && sm_doors_plm_direction(plm_header_ptr[v1],0)<0)',1),
    ('if ((collected_items & 0x1000) != 0)', 'if (sm_world_torizo_wakes())',2),
    ('  if ((collected_items & 0x200) != 0\n      && (samus_collision_direction & 0xF) == 3',
     '  if (((collected_items & 0x200) != 0 || sm_world_chozo_without_spacejump())\n      && (samus_collision_direction & 0xF) == 3',1),
    ('  SaveToSram(selected_save_slot);', '  sm_refill_at_save();\n  sm_visual_sprite(samus_x_pos,samus_y_pos,142);\n  SaveToSram(selected_save_slot);',1),
    ('void LoadRoomPlmGfx(void) {', 'void LoadRoomPlmGfx(void) {\n  sm_relic_reload(1);',1),
    ('  } while (v0 != 8);\n}\n\nvoid ClearSoundsWhenGoingThroughDoor', '  } while (v0 != 8);\n  sm_relic_reload(0);\n}\n\nvoid ClearSoundsWhenGoingThroughDoor',1),
    ('  v5->src.addr = GET_WORD(plmp);', '  v5->src.addr = sm_relic_gfx(k,GET_WORD(plmp),v3);',1),
    ('  samus_max_reserve_health += GET_WORD(plmp);', '  if(sm_relic_collect(k))return plmp+2;\n  samus_max_reserve_health += GET_WORD(plmp);',1),
    ('const uint8 *PlmInstr_PickupEquipmentAndShowMessage(const uint8 *plmp, uint16 k) {  // 0x8488F3\n  uint16 t = GET_WORD(plmp);',
     'const uint8 *PlmInstr_PickupEquipmentAndShowMessage(const uint8 *plmp, uint16 k) {  // 0x8488F3\n  uint16 t = sm_seed_equipment_mask(plmp,k);',1),
    ('  samus_max_reserve_health += GET_WORD(plmp);', '  samus_max_reserve_health += GET_WORD(plmp);\n  if(sm_varia_ui_active(SM_VARIA_RESERVES))samus_reserve_health=samus_max_reserve_health;',1),

    ('    SetEventHappened(0);','    if (!sm_seed_active()) SetEventHappened(0);',1),
])

write('85',[
    ('  CancelSoundEffects();','  if(!sm_seed_rule(SM_SEED_ITEM_SOUNDS))CancelSoundEffects();',1),
    ('      my_counter = 360;','      my_counter = sm_seed_rule(SM_SEED_ITEM_SOUNDS)?32:360;',1),

    ('  uint16 r0 = kMessageBoxDefs[message_box_index - 1].message_tilemap;',
     '  if(sm_relic_message()){sm_relic_message_tiles(ram3000.pause_menu_map_tilemap+288);message_box_das0l_value=384;return 160;}\n  uint16 r0 = kMessageBoxDefs[message_box_index - 1].message_tilemap;',1),
    ('static void InitializeMessageBox(void) {',
     'static void InitializeMessageBox(void) {\n  if(sm_relic_message()){WriteLargeMessageBoxTilemap();SetupPpuForLargeMessageBox();return;}',1),
    ('  RestorePpuForMessageBox();',
     '  RestorePpuForMessageBox();\n  sm_relic_message_closed();',1),
])

write('b4',[
    ('  sprite_x_pos[v1] = x_r18;',
     '  sm_visual_sprite(x_r18,y_r20,ilist_r22);\n  sprite_x_pos[v1] = x_r18;',1),
])
write('a0',[
    ('    EnemyData *E = gEnemyData(v5);', '    EnemyData *E = gEnemyData(v5);\n    sm_run_enemy_spawn(v5);',1),
    ('    EnemyData *E = gEnemyData(new_enemy_index);', '    EnemyData *E = gEnemyData(new_enemy_index);\n    sm_run_enemy_spawn(new_enemy_index);',1),
    ('uint16 SuitDamageDivision(uint16 a) {','uint16 SuitDamageDivision(uint16 a) {\n  a=sm_run_contact_damage(a);',1),
    ('      E->health = (int16)(E->health - dmg)', '      dmg=sm_run_enemy_damage(dmg);\n      E->health = (int16)(E->health - dmg)',1),
    ('        uint16 health = v6->health;', '        ttt=sm_run_enemy_damage(ttt);\n        uint16 health = v6->health;',1),
    ('      uint16 health = v16->health;', '      pd=sm_run_enemy_damage(pd);\n      uint16 health = v16->health;',1),
    ('uint16 SuitDamageDivision(uint16 a) {', 'uint16 SuitDamageDivision(uint16 a) {\n  if(sm_seed_rule(SM_SEED_PROGRESSIVE_SUITS))return a >> (((equipped_items&1)!=0)+((equipped_items&0x20)!=0));',1),
    ('    r22 = 200;','    r22 = sm_seed_rule(SM_SEED_NERFED_CHARGE) && !(equipped_beams&0x1000)?66:200;',1),
    ('void CallEnemyAi(uint32 ea) {','void sm_original_CallEnemyAi(uint32 ea) {',1),
    ('    RecordEnemySpawnData(v5);','    E->extra_properties=sm_objective_enemy_properties(0xa1,v4,E->extra_properties);\n    RecordEnemySpawnData(v5);',1),
    ('    E->ai_var_A = 0;','    E->extra_properties=sm_objective_enemy_properties(db,varE20,E->extra_properties);\n    E->ai_var_A = 0;',1),
    ('(int16)(v1->x_width + v1->x_pos - layer1_x_pos) >= 0', '(int16)(v1->x_width + v1->x_pos - layer1_x_pos + sm_view_margin()) >= 0',1),
    ('(int16)(v1->x_width + layer1_x_pos + 256 - v1->x_pos) >= 0', '(int16)(v1->x_width + layer1_x_pos + 256 + sm_view_margin() - v1->x_pos) >= 0',1),
    ('(int16)(E->x_width + E->x_pos - layer1_x_pos) < 0', '(int16)(E->x_width + E->x_pos - layer1_x_pos + sm_view_margin()) < 0',1),
    ('(int16)(E->x_width + layer1_x_pos + 256 - E->x_pos) < 0', '(int16)(E->x_width + layer1_x_pos + 256 + sm_view_margin() - E->x_pos) < 0',1),
    ('(int16)(v0->x_pos - layer1_x_pos) < 0 || (int16)(layer1_x_pos + 256 - v0->x_pos) < 0',
     '(int16)(v0->x_pos - layer1_x_pos + sm_view_margin()) < 0 || (int16)(layer1_x_pos + 256 + sm_view_margin() - v0->x_pos) < 0',1),

    ('void EnemyDeathAnimation(uint16 k, uint16 a) {',
     'void EnemyDeathAnimation(uint16 k, uint16 a) {\n  sm_objective_enemy_death(cur_enemy_index);\n  sm_visual_sprite(gEnemyData(cur_enemy_index)->x_pos,gEnemyData(cur_enemy_index)->y_pos,128);\n  if(samus_contact_damage_index==1 || samus_contact_damage_index==2)sm_visual_sprite(gEnemyData(cur_enemy_index)->x_pos,gEnemyData(cur_enemy_index)->y_pos,144);',1),
])

write('86',[
    ('if ((int16)(eproj_x_pos[v1] - layer1_x_pos) >= 0) {',
     'if ((int16)(eproj_x_pos[v1] - layer1_x_pos + sm_view_margin()) >= 0) {',1),
    ('if ((int16)(eproj_x_pos[v1] - (layer1_x_pos + 256)) < 0',
     'if ((int16)(eproj_x_pos[v1] - (layer1_x_pos + 256 + sm_view_margin())) < 0',1),
])

# Cache the winning layer during the exact reference color decode. Presentation
# metadata reuses this result instead of decoding the same pixel a second time.
ppu=(source/'snes/ppu.c').read_text()
needle='    int mainLayer = ppu_getPixel(ppu, x, y, false, &r, &g, &b);'
assert ppu.count(needle)==1
ppu=ppu.replace(needle,needle+'\n    sm_last_main_layer[x]=(uint8_t)mainLayer;')
needle='static bool ppu_evaluateSprites(Ppu* ppu, int line) {'
assert ppu.count(needle)==1
ppu=ppu.replace(needle,needle+'\n  memset(sm_last_sprite_owner,255,sizeof(sm_last_sprite_owner));')
needle='              if (pixel != 0 && (dst[0] & 0xff) == 0)\n                dst[0] = z + pixel;'
assert ppu.count(needle)==1
ppu=ppu.replace(needle,'              if (pixel != 0 && (dst[0] & 0xff) == 0) {\n                dst[0] = z + pixel;\n                if(col+x+px>=0 && col+x+px<256)sm_last_sprite_owner[col+x+px]=index;\n              }')
for name in ('ppu.h','snes.h','../types.h'):
    ppu=ppu.replace('#include "'+name+'"','#include "'+str((source/'snes'/name).resolve())+'"')
(out/'sm_ppu_decoder.c').write_text('static unsigned char sm_last_main_layer[256],sm_last_sprite_owner[256];\n'+ppu)

# Space Jump assistance retains the original branch verbatim when disabled.
movement=out/'sm_90_movement.c'
text=movement.read_text()
start=text.index('void Samus_Movement_03_SpinJumping(void) {')
left=text.index('    if ((equipped_items & 0x200) != 0) {',start)
right=text.index('LABEL_24:;',left)
original=text[left:right]
text=text[:left]+('    if (sm_assisted_spacejump()) {\n'
    '      if (sm_spacejump_request()) Samus_InitJump();\n'
    '    } else {\n'+original+'    }\n')+text[right:]
text=text.replace('void Samus_Movement_03_SpinJumping(void) {',
    'void Samus_Movement_03_SpinJumping(void) {\n  sm_spacejump_tick();',1)
text=text.replace('void Samus_FootstepGraphics(void) {',
    'void Samus_FootstepGraphics(void) {\n  sm_visual_footstep();',1)
movement.write_text('#include "sm_spacejump.h"\n'+text)

write('8b',[
    ('void CinematicFunction_Intro_Initial(void) {','void CinematicFunction_Intro_Initial(void) {\n  sm_cinema_begin(1);',1),
    ('void CinematicFunction_Intro_Func54(void) {','void CinematicFunction_Intro_Func54(void) {\n  sm_cinema_begin(2);',1),
    ('    QueueMode7Transfers(0x8b, addr_kCinematicFunction_Intro_Func56_M7);','    sm_cinema_begin(8);\n    QueueMode7Transfers(0x8b, addr_kCinematicFunction_Intro_Func56_M7);',1),
    ('    QueueMode7Transfers(0x8b, addr_kCinematicFunction_Intro_Func76_M7_1);','    sm_cinema_begin(game_state==37?0:9);\n    QueueMode7Transfers(0x8b, addr_kCinematicFunction_Intro_Func76_M7_1);',1),
    ('void CinematicFunctionBlackoutFromCeres(void) {','void CinematicFunctionBlackoutFromCeres(void) {\n  sm_cinema_begin(3);',1),
    ('void CinematicFunction_Intro_Func86(void) {','void CinematicFunction_Intro_Func86(void) {\n  sm_cinema_begin(4);',1),
    ('void CinematicFunctionEscapeFromCebes(void) {','void CinematicFunctionEscapeFromCebes(void) {\n  sm_cinema_begin(5);',1),
    ('void CinematicFunction_Intro_Func112(void) {','void CinematicFunction_Intro_Func112(void) {\n  sm_cinema_begin(6);',1),
    ('void CinematicFunction_Intro_Func120(void) {','void CinematicFunction_Intro_Func120(void) {\n  sm_cinema_begin(7);',1),
    ('void CinematicFunction_Intro_Func126(void) {','void CinematicFunction_Intro_Func126(void) {\n  sm_cinema_begin(0);\n  if(sm_relic_ending()){QueueMusic_Delayed8(0xFF3C);QueueMusic_DelayedY(5,0xE);}',1),
    ('uint8 SpawnCimenaticSpriteObjectInner(uint16 k, uint16 j) {','uint8 SpawnCimenaticSpriteObjectInner(uint16 k, uint16 j) {\n  sm_cinema_sprite(j>>1,k);',1),

    ('void CreditsObject_Process(void) {', 'void CreditsObject_Process(void) {\n  if(sm_credits_native_tick())return;',1),
    ('  CreditsObject_Init(addr_stru_8BF6F8);','  CreditsObject_Init(addr_stru_8BF6F8);\n  sm_credits_start_ending();',1),
])
write('a2',[
    ('void GunshipTop_12(uint16 k) {', 'void GunshipTop_12(uint16 k) {',1),
    ('      E->gtp_var_F = FUNC16(GunshipTop_9);','      if(!sm_relic_required() && CheckEventHappened(0xE))sm_stats_finish();\n      E->gtp_var_F = FUNC16(GunshipTop_9);',1),
])

# Caption sprites retain their original color bytes above atmosphere effects.
p=out/'sm_8b_native.c';text=p.read_text()
for name in ['DrawCinematicSpriteObjects_Intro','DrawCinematicSpriteObjects_Ending']:
    a=text.index('void '+name+'(void) {');b=text.index('\n}\n',a)
    body=text[a:b];needle='      uint16 chr = cinematicbg_arr9[v1];'
    assert body.count(needle)==1
    body=body.replace(needle,needle+'\n      sm_sprite_view_set_gui(sm_cinema_sprite_gui(v1));')
    text=text[:a]+body+'\n  sm_sprite_view_set_gui(0);'+text[b:]
p.write_text(text)

# Relic quota completion uses the real ship takeoff and native ending cleanup.
p=out/'sm_a2_native.c';text=p.read_text()
a=text.index('void GunshipTop_12(uint16 k) {');b=text.index('\n}\n',a)
body=text[a:b];assert body.count('if (CheckEventHappened(0xE)) {')==1
body=body.replace('if (CheckEventHappened(0xE)) {','if (sm_relic_required() ? sm_relic_depart() : CheckEventHappened(0xE)) {')
p.write_text(text[:a]+body+text[b:])

# Native equivalent of VARIA nerfed_rainbow_beam.asm: 20 drain frames,
# 40 damage without Varia / 20 with Varia. Original behavior is 300 frames.
write('a9',[
    ('  E->mbn_var_A = FUNC16(MotherBomb_FiringRainbowBeam_1_StartCharge);',
     '  E->mbn_var_A = sm_tourian_fast()?FUNC16(MotherBrain_Phase3_Recover_MakeDistance):FUNC16(MotherBomb_FiringRainbowBeam_1_StartCharge);',1),
    ('    E->mbn_var_A = FUNC16(MotherBrain_Phase3_Death_11);',
     '    sm_tourian_hyper_start();\n    E->mbn_var_A = FUNC16(MotherBrain_Phase3_Death_11);',1),
    ('    E->mbn_var_A = FUNC16(MotherBrain_Phase3_Death_13);',
     '    sm_tourian_hyper_end();\n    E->mbn_var_A = FUNC16(MotherBrain_Phase3_Death_13);',1),
    ('void MotherBrain_Pal_ProcessInvincibility(void) {',
     'void MotherBrain_Pal_ProcessInvincibility(void) {\n  if(sm_tourian_fast())return;',1),

    ('  E->mbb_var_F = 299;\n  earthquake_timer = 299;',
     '  E->mbb_var_F = sm_seed_rule(SM_SEED_NERFED_RAINBOW)?19:299;\n  earthquake_timer = E->mbb_var_F;',1),
])

# lioran's VARIA elevators_speed.asm: 3 pixels per frame. Downward
# departure moves for at most 24 transition frames, preventing overshoot.
# The original patch aliases pause-menu RAM $0741; use the same saved RAM
# so resetting/loading native state cannot leave an external timer behind.
write('a3',[
    ('  if ((equipped_items & 0x20) != 0) {\n    v1 = 12288;', '  if(sm_seed_rule(SM_SEED_PROGRESSIVE_SUITS)){\n    v1=0xc000 >> (((equipped_items&1)!=0)+((equipped_items&0x20)!=0));\n  } else if ((equipped_items & 0x20) != 0) {\n    v1 = 12288;',1),
    ('-0x18000);', '-(sm_seed_rule(SM_SEED_FAST_ELEVATORS)?0x30000:0x18000));',2),
    ('    AddToHiLo(&E->base.y_pos, &E->base.y_subpos, 0x18000);',
     '    AddToHiLo(&E->base.y_pos, &E->base.y_subpos, sm_seed_rule(SM_SEED_FAST_ELEVATORS)?0x30000:0x18000);',2),
    ('    elevator_direction = 0;\n    AddToHiLo(&E->base.y_pos, &E->base.y_subpos, sm_seed_rule(SM_SEED_FAST_ELEVATORS)?0x30000:0x18000);',
     '    elevator_direction = 0;\n    if (sm_seed_rule(SM_SEED_FAST_ELEVATORS)) {\n      if (game_state==8) pausemenu_item_selector_animation_frame=0;\n      else if (pausemenu_item_selector_animation_frame==24) {Elevator_Func_4();return;}\n      else ++pausemenu_item_selector_animation_frame;\n      E->base.y_pos+=3;\n    } else AddToHiLo(&E->base.y_pos, &E->base.y_subpos, 0x18000);',1),
])

write('aa',[
    ('  *(uint16 *)&scrolls[13] = 1;','  *(uint16 *)&scrolls[13] = 1;\n  sm_objective_event_mark(145); /* VARIA bowling_chozo_event */',1),
])

# VARIA conditionally marks the four statue events, advancing normally.
write('87', [('  SetEventHappened(*v2);',
  '  if (sm_objectives_statue_event(j)) SetEventHappened(*v2);',1)])

# objectives.asm redirects SPC $38D0 from $399D to $39A8: retain both
# particle-sound voices and set priority 1. Translate the native SPC switch,
# with the same applied-plan lifetime, leaving the upstream player pristine.
spc=(source/'spc_player.c').read_text()
old='switch (kSfx2Conf[a - 1]) {'
assert spc.count(old)==1
spc=spc.replace(old,'switch (a == 0x19 && sm_objectives_state(0) ? 3 : kSfx2Conf[a - 1]) {')
(out/'spc_player_native.c').write_text('#include "sm_objectives.h"\n'+spc)

# Original VARIA Scavenger appearance/death seams. Ceres and non-Scavenger
# seeds retain the pristine translated timing and routines.
write('a6',[
    ('QueueMusic_Delayed8(3);','if(!sm_escape_active())QueueMusic_Delayed8(3);',1),
    ('  if ((--E->cry_var_F & 0x8000) != 0) {\n    E->cry_var_A = FUNC16(CeresRidley_Func_4);',
     '  if (sm_scavenger_state(0) && area_index==2) {\n    if (--E->cry_var_F) return;\n    if (!sm_scavenger_ridley_appears()) {++E->cry_var_F;return;}\n    E->cry_var_A=FUNC16(CeresRidley_Func_4);E->cry_var_E=E->cry_var_F=0;return;\n  }\n  if ((--E->cry_var_F & 0x8000) != 0) {\n    E->cry_var_A = FUNC16(CeresRidley_Func_4);',1),
    ('  if ((--E->ridley_var_F & 0x8000) != 0) {\n    if (E->ridley_var_1B)',
     '  if ((--E->ridley_var_F & 0x8000) != 0) {\n    sm_scavenger_ridley_dead();\n    if(sm_minimizer_active())SetBossBitForCurArea(1);\n    if (E->ridley_var_1B)',1),
])

# minimizer_bosses.asm: mark exactly the original patched death seams and
# preserve the mixed-domain blinking PLM in Phantoon's doorway.
write('a7',[
    ('QueueMusic_Delayed8(3);','if(!sm_escape_active())QueueMusic_Delayed8(3);',2),
    ('  if (!sign16(E->kraid_var_A + 0x3AC9))',
     '  if(sm_minimizer_active() && E->kraid_var_A==0xc537)SetBossBitForCurArea(1);\n  if (!sign16(E->kraid_var_A + 0x3AC9))',1),
    ('  if (!EK->base.health) {\n    QueueSfx2_Max6(0x73);',
     '  if (!EK->base.health) {\n    QueueSfx2_Max6(0x73);\n    if(sm_minimizer_active())SetBossBitForCurArea(1);',1),
    ('      SpawnHardcodedPlm((SpawnHardcodedPlmArgs) { 0x00, 0x06, 0xb781 });',
     '      if(!sm_minimizer_active())SpawnHardcodedPlm((SpawnHardcodedPlmArgs) { 0x00, 0x06, 0xb781 });',1),
    ('      SpawnHardcodedPlm((SpawnHardcodedPlmArgs) { 0x00, 0x06, 0xb78b });',
     '      if(!sm_minimizer_active())SpawnHardcodedPlm((SpawnHardcodedPlmArgs) { 0x00, 0x06, 0xb78b });',1),
])
write('a5',[
    ('      int16 v9 = E->base.health - 256;', '      int16 v9 = E->base.health - sm_run_enemy_damage(256);',1),
    ('      uint16 v6 = addr_kDraygon_Ilist_999C;\n      if (E->draygon_var_20)',
     '      uint16 v6 = addr_kDraygon_Ilist_999C;\n      if(sm_minimizer_active())SetBossBitForCurArea(1);\n      if (E->draygon_var_20)',1),
])

write('91',[
    ('      samus_health -= a;', '      a = sm_relic_escape_damage(a);\n      samus_health -= a;',1),
    ('    PoseEntry *pe = get_PoseEntry(kPoseTransitionTable[samus_pose]);',
     '    const PoseEntry *pe = sm_seed_rule(SM_SEED_RESPIN)?(const PoseEntry*)sm_seed_pose_entries(samus_pose):get_PoseEntry(kPoseTransitionTable[samus_pose]);',1),
    ('void HandleJumpTransition_SpinJump(void) {',
     'void HandleJumpTransition_SpinJump(void) {\n  if(sm_seed_rule(SM_SEED_RESPIN) && (samus_prev_movement_type2==2 || samus_prev_movement_type2==6))return;',1),

    ('uint8 Samus_HandleTransFromBlockColl_1_0(void) {','uint8 sm_original_landing_transition(void) {',1),
])
with (out/'sm_91_native.c').open('a') as f:
    f.write('\nint sm_seed_landing_eligible(void){return samus_prev_movement_type2==3 || samus_prev_movement_type2==20 || kPoseParams[samus_pose].direction_shots_fired!=255;}\n')

write('8d',[
    ('  if ((equipped_items & 0x21) == 0) {',
     '  if ((equipped_items & (sm_seed_rule(SM_SEED_BALANCED_SUITS|SM_SEED_PROGRESSIVE_SUITS)?1:0x21)) == 0) {',1),
])

write('b3',[
    ('E->base.properties |= kEnemyProps_DisableSamusColl | kEnemyProps_Tangible | 0x8000;',
     'E->base.properties |= sm_animals_hostile()?0xa000:(kEnemyProps_DisableSamusColl | kEnemyProps_Tangible | 0x8000);',1),
])
