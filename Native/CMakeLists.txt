cmake_minimum_required(VERSION 3.20)
project(SMNative C)
set(CMAKE_C_STANDARD 17)
find_package(Python3 REQUIRED COMPONENTS Interpreter)
if(WIN32 AND NOT MINGW)
  message(FATAL_ERROR "The native C core requires MinGW-w64/UCRT on Windows; Unreal uses MSVC separately.")
endif()
file(GLOB GAME_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../native-core/src/sm_*.c")
# Keep the upstream checkout pristine: replace one definition in a generated
# translation unit so both original and assisted wall-jump paths are available.
file(READ "${CMAKE_CURRENT_SOURCE_DIR}/../native-core/src/sm_90.c" SM_MOVEMENT_SOURCE)
string(REPLACE "uint8 Samus_WallJumpCheck(int32 amt)" "uint8 sm_original_wall_jump_check(int32 amt)" SM_MOVEMENT_SOURCE "${SM_MOVEMENT_SOURCE}")
string(REPLACE "    r18 = v5;\n    if ((v5 & 0xFF00) == 0)"
  "    sm_visual_charge(r20+layer1_x_pos,v5+layer1_y_pos);\n    r18 = v5;\n    if ((v5 & 0xFF00) == 0)" SM_MOVEMENT_SOURCE "${SM_MOVEMENT_SOURCE}")
string(REPLACE "if (!sign16(v0 + 64)) {\n    if (sign16(v0 - 320)) {"
  "if (!sign16(v0 + 64 + sm_view_margin())) {\n    if (sign16(v0 - 320 - sm_view_margin())) {" SM_MOVEMENT_SOURCE "${SM_MOVEMENT_SOURCE}")
string(REPLACE "      InitializeBombExplosion(v0);"
  "      sm_visual_sprite(projectile_x_pos[v1],projectile_y_pos[v1],24);\n      InitializeBombExplosion(v0);" SM_MOVEMENT_SOURCE "${SM_MOVEMENT_SOURCE}")
# The native minimap uses full authored knowledge in randomized sessions while
# retaining real exploration bits for colors and gameplay/save behavior.
string(REPLACE "const uint8 *r9 = RomPtr_82(kPauseMenuMapData[area_index]);"
  "const uint8 *r9 = sm_seed_map_data(area_index);" SM_MOVEMENT_SOURCE "${SM_MOVEMENT_SOURCE}")
string(REPLACE "!has_area_map)" "!(has_area_map || sm_map_fully_known()))" SM_MOVEMENT_SOURCE "${SM_MOVEMENT_SOURCE}")
string(REPLACE "if (!sign16(samus_health - 30) && !samus_collision_flag)"
  "if (!sign16(sm_connections_spark_health() - 30) && !samus_collision_flag)" SM_MOVEMENT_SOURCE "${SM_MOVEMENT_SOURCE}")
file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/sm_90_movement.c" "#include \"sm_connections.h\"\n#include \"sm_effects.h\"\n#include \"sm_seed.h\"\n${SM_MOVEMENT_SOURCE}")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare-map-exploration.py" "${CMAKE_CURRENT_BINARY_DIR}/sm_90_movement.c" COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare-stats-movement.py" "${CMAKE_CURRENT_BINARY_DIR}/sm_90_movement.c" COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare-seed-gameplay.py" "${CMAKE_CURRENT_BINARY_DIR}/sm_90_movement.c" COMMAND_ERROR_IS_FATAL ANY)
list(FILTER GAME_SOURCES EXCLUDE REGEX "/sm_90\\.c$")
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare_overlays.py"
  "${CMAKE_CURRENT_SOURCE_DIR}/../native-core/src" "${CMAKE_CURRENT_BINARY_DIR}"
  COMMAND_ERROR_IS_FATAL ANY)
execute_process(COMMAND "${Python3_EXECUTABLE}" "${CMAKE_CURRENT_SOURCE_DIR}/prepare-mirror-gameplay.py" "${CMAKE_CURRENT_BINARY_DIR}" COMMAND_ERROR_IS_FATAL ANY)
foreach(BANK b3 a5 a7 a6 a3 a9 80 81 a8 8f 82 84 85 b4 a0 86 8b a2 aa 87 91 8d)
  list(FILTER GAME_SOURCES EXCLUDE REGEX "/sm_${BANK}\\.c$")
  list(APPEND GAME_SOURCES "${CMAKE_CURRENT_BINARY_DIR}/sm_${BANK}_native.c")
endforeach()
file(GLOB HARDWARE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/../native-core/src/snes/*.c")
list(FILTER HARDWARE_SOURCES EXCLUDE REGEX "/ppu\\.c$")
add_library(sm_native SHARED sm_animals.c sm_mirror.c sm_escape.c sm_minimizer.c sm_tourian.c sm_scavenger.c sm_objectives.c sm_objective_events.c sm_map_exploration.c sm_map_icons.c sm_areas.c sm_doors.c sm_connections.c sm_indicators.c sm_start.c sm_world_data.c sm_seed_rules.c sm_save_refill.c sm_relic.c sm_bridge.c sm_scene.c sm_sprite_view.c sm_walljump.c sm_spacejump.c sm_randomizer.c sm_seed.c sm_generation.c sm_tracker.c sm_varia_ui.c sm_credits.c sm_cinematics.c sm_run_stats.c sm_effects.c sm_pause.c sm_map_browser.c sm_travel.c sm_decor.c "${CMAKE_CURRENT_BINARY_DIR}/sm_90_movement.c" ${GAME_SOURCES} ${HARDWARE_SOURCES}
 "${CMAKE_CURRENT_BINARY_DIR}/spc_player_native.c" ../native-core/src/tracing.c ../native-core/src/util.c)
target_sources(sm_native PRIVATE sm_objective_pause.c sm_soundtrack.c sm_gameover.c sm_notifications.c sm_relic_escape.c sm_runs.c sm_route.c sm_item_selection.c)
target_include_directories(sm_native PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}" "${CMAKE_CURRENT_BINARY_DIR}")
target_compile_options(sm_native PRIVATE -iquote${CMAKE_CURRENT_SOURCE_DIR}/../native-core/src -O3 -fno-strict-aliasing -fvisibility=hidden)
target_link_options(sm_native PRIVATE -Wl,--no-undefined -Wl,--wrap=cpu_runOpcode)
target_compile_definitions(sm_native PRIVATE SM_NATIVE_BUILD)
if(WIN32)
  target_sources(sm_native PRIVATE sm_windows.c)
  target_compile_options(sm_native PRIVATE -include${CMAKE_CURRENT_SOURCE_DIR}/sm_platform_io.h)
  target_link_libraries(sm_native PRIVATE m bcrypt)
  target_link_options(sm_native PRIVATE -static-libgcc)
  set_target_properties(sm_native PROPERTIES PREFIX "" OUTPUT_NAME "sm_native")
else()
  target_link_libraries(sm_native PRIVATE m dl pthread)
endif()
add_executable(sm_randomizer_host test_randomizer_host.c)
target_link_libraries(sm_randomizer_host PRIVATE sm_native)
