#include "sm_animals.h"
#include "sm_escape.h"
#include "sm_objective_pause.h"
#include "sm_soundtrack.h"
#include "sm_gameover.h"
#include "sm_runs.h"
#include "sm_route.h"
#include "sm_notifications.h"
#include "sm_tourian.h"
#include "sm_objectives.h"
#include "sm_connections.h"
#include "sm_areas.h"
#include "sm_start.h"
#include "sm_relic.h"
#include "sm_item_selection.h"
#include "sm_generation.h"
#include "sm_credits.h"
#include "sm_map_icons.h"
#include "sm_cinematics.h"
#include "sm_run_stats.h"
#include "sm_tracker.h"
#include "sm_world_data.h"
#include "sm_map_browser.h"
#include "sm_travel.h"
#include "sm_varia_ui.h"
#include "sm_bridge.h"
#include "sm_scene.h"
#include "sm_walljump.h"
#include "sm_spacejump.h"
#include "sm_seed.h"
#include "sm_effects.h"
#include "sm_profile.h"
uint64_t sm_profile_ns[5];
double sm_profile_us(int field){return field>=0&&field<5?sm_profile_ns[field]/1000.0:0;}
#include "sm_cpu_infra.h"
#include "sm_rtl.h"
#include "ida_types.h"
#include "spc_player.h"
#include "variables.h"
#include "funcs.h"
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>

bool g_debug_flag, g_is_turbo, g_other_image;
bool g_new_ppu = false;
int g_got_mismatch_count;
SpcPlayer *g_spc_player;
extern uint8 g_runmode;
static uint8_t pixels[256 * 240 * 4];
static int16_t audio_samples[736 * 2];
static char last_error[512], save_path[4096];
static uint64_t cpu_opcodes;
static jmp_buf recovery;
static int guarded, initialized, defer_sram_write;
static int test_room_pending,test_room_x,test_room_y,test_room_door;

void Die(const char *message) {
  snprintf(last_error, sizeof(last_error), "%s", message);
  if (guarded) longjmp(recovery, 1);
  abort();
}
void Warning(const char *message) { fprintf(stderr, "SM warning: %s\n", message); }
/* Core and audio run on the same Unreal game thread. */
void RtlApuLock(void) {}
void RtlApuUnlock(void) {}
void sm_soundtrack_spc_command(uint8_t command) { RtlApuWrite(APUI00, command); }
void __wrap_cpu_runOpcode(Cpu *cpu) {
  (void)cpu;
  ++cpu_opcodes;
  Die("Native-only guard: an emulated 65816 opcode was requested.");
}
int sm_init(const char *rom_path, const char *sram_path) {
  if (initialized) { snprintf(last_error, sizeof(last_error), "Core already initialized"); return 0; }
  last_error[0] = 0;
  if (!rom_path || !sram_path || strlen(sram_path) >= sizeof(save_path)) return 0;
  if (sm_seed_active() && !strstr(sram_path,sm_seed_fingerprint())) {
    snprintf(last_error,sizeof(last_error),"Randomized sessions require a save path containing the seed fingerprint");return 0;
  }
  FILE *f = fopen(rom_path, "rb");
  if (!f) { snprintf(last_error, sizeof(last_error), "Cannot open ROM"); return 0; }
  fseek(f, 0, SEEK_END); long size = ftell(f); rewind(f);
  if (size != 3145728) { fclose(f);snprintf(last_error, sizeof(last_error), "Expected unheadered Japan/USA ROM (3145728 bytes)"); return 0; }
  /* Native callers cannot bypass the game's startup compatibility gate. */
  uint32_t crc=0xffffffffu;unsigned char block[4096];size_t got,total=0;
  while((got=fread(block,1,sizeof(block),f))){
    total+=got;
    for(size_t i=0;i<got;i++){
      crc^=block[i];
      for(int bit=0;bit<8;bit++)crc=(crc>>1)^(0xedb88320u&-(crc&1u));
    }
  }
  fclose(f);crc^=0xffffffffu;
  if(total!=3145728 || crc!=0xd63ed5f8u){
    snprintf(last_error,sizeof(last_error),"Incompatible ROM CRC32 %08X; expected D63ED5F8",crc);return 0;
  }
  guarded = 1;
  if (setjmp(recovery)) { guarded = 0; return 0; }
  memset(g_ram, 0, sizeof(g_ram));
  test_room_pending=test_room_door=0;
  snes_frame_counter=0;
  if (!SnesInit(rom_path)) { guarded = 0; return 0; }
  sm_soundtrack_reset();
  sm_credits_load_assets();
  sm_world_data_load_assets();
  sm_areas_load_assets();
  sm_escape_load_assets();
  sm_animals_load_assets();
  sm_varia_ui_load_assets();
  sm_objective_pause_load_assets();
  sm_tracker_load_assets();
  sm_slots_reset();sm_tracker_configure(0,1,1);sm_seed_capture_original();
  if (!sm_seed_apply((uint8_t*)g_rom)) { Die("Seed item table does not match ROM visibility"); }
  g_spc_player = SpcPlayer_Create();
  SpcPlayer_Initialize(g_spc_player);
  g_runmode = 1; /* RM_MINE: C game logic exclusively. */
  g_snes->disableRender = false;
  g_snes->ppu = g_snes->my_ppu;
  PpuBeginDrawing(g_snes->my_ppu, pixels, 256 * 4, 0);
  PpuBeginDrawing(g_snes->snes_ppu, pixels, 256 * 4, 0);
  snprintf(save_path, sizeof(save_path), "%s", sram_path);
  f = fopen(save_path, "rb");
  if (f) { size_t n = fread(g_sram, 1, 8192, f); fclose(f); if (n != 8192) memset(g_sram, 0, 8192); }
  cpu_opcodes = 0;
  sm_credits_reset();sm_cinema_begin(0);sm_stats_open(save_path);sm_relic_escape_open(save_path);sm_runs_open(save_path);sm_route_open(save_path);
  sm_scene_reset();sm_notifications_reset();sm_visual_reset();
  sm_walljump_reset();sm_spacejump_reset();sm_item_selection_reset();
  initialized = 1;sm_seed_running(1);
  sm_generation_configure(sm_seed_active(),sm_seed_active());sm_tracker_new_session();
  guarded = 0;
  return 1;
}
int sm_step(uint16_t buttons) {
  if (!initialized || last_error[0]) return 0;
  guarded = 1;
  if (setjmp(recovery)) { guarded = 0; return 0; }
  if(sm_credits_preview_step(buttons,pixels,audio_samples)){guarded=0;return 1;}
  sm_scene_active=sm_presentation_kind()==1;
  sm_visual_begin_frame();
  sm_seed_frame();
  memset(sm_profile_ns,0,sizeof(sm_profile_ns));
  uint64_t profile_start=sm_clock_ns();
  sm_spacejump_frame();
  RtlRunFrame(sm_walljump_input(sm_item_selection_input(buttons)));
  /* RM_MINE bypasses the upstream comparison runner's soft-reset hook.
   * Resume the native reset coroutine, preserving SRAM and avoiding an
   * out-of-range game-state dispatch after Game Over -> End. */
  if(game_state==0xffff)coroutine_state_0=3;
  sm_profile_ns[0]=sm_clock_ns()-profile_start;
  sm_runs_frame();
  sm_relic_escape_frame();
  sm_visual_end_frame();
  sm_seed_frame();
  sm_objectives_frame();
  sm_tourian_frame();
  sm_stats_frame();sm_route_frame();
  RtlRenderAudio(audio_samples, 736, 2);
  sm_soundtrack_mix(audio_samples,736);
  for (int i = 3; i < (int)sizeof(pixels); i += 4) pixels[i] = 255;
  profile_start=sm_clock_ns();
  sm_scene_finish();
  sm_varia_ui_frame();
  if(!sm_map_browser_overview()){sm_varia_ui_render(pixels);sm_map_icons_render(pixels);sm_tracker_render(pixels);}
  sm_objective_map_render(pixels);
  sm_relic_render(pixels);
  sm_map_browser_render(pixels);sm_travel_render(pixels);
  sm_credits_render(pixels);sm_cinema_frame();
  sm_hud_stabilize(pixels);
  sm_run_render(pixels);
  sm_generation_render(pixels);
  sm_notifications_frame(pixels);
  sm_profile_ns[4]=sm_clock_ns()-profile_start;
  guarded = 0;
  return 1;
}
const uint8_t *sm_background_mask(void) { return sm_scene_background_mask; }
const uint8_t *sm_emission(void) { return sm_scene_emission; }
const uint8_t *sm_lightmap(void) { return sm_scene_lights; }
int sm_parallax_supported(void) { return (g_snes->ppu->mode==7 && room_ptr==0xdf45) || (g_snes->ppu->mode==1 && (room_ptr==0x91f8 || (layer2_scroll_x>1 && layer2_scroll_y!=1))); }
const uint8_t *sm_scene(void) { return sm_scene_pixels; }
const uint8_t *sm_layers(void) { return sm_scene_layers; }
void sm_set_parallax(int x,int y,int enabled) { sm_scene_camera_x=x; sm_scene_camera_y=y; sm_scene_parallax=enabled; }
const uint8_t *sm_vram(void) { return (const uint8_t*)g_snes->ppu->vram; }
const uint16_t *sm_palette(void) { return g_snes->ppu->cgram; }
const uint8_t *sm_oam(void) { return (const uint8_t*)g_snes->ppu->oam; }
int sm_ppu_mode(void) { return g_snes->ppu->mode; }
int sm_bg_info(int layer,int field) {
  const BgLayer *b=&g_snes->ppu->bgLayer[layer&3];
  switch(field) { case 0:return b->tileAdr; case 1:return b->tilemapAdr; case 2:return b->hScroll; case 3:return b->vScroll; case 4:return b->tilemapWider; case 5:return b->tilemapHigher; }
  return 0;
}
const uint8_t *sm_pixels(void) { return pixels; }
const int16_t *sm_audio(void) { return audio_samples; }
const char *sm_error(void) { return last_error; }
int sm_state(void) { return game_state; }
int sm_frame(void) { return snes_frame_counter; }
uint64_t sm_cpu_opcodes(void) { return cpu_opcodes; }
uint64_t sm_simulation_hash(void) {
  uint64_t h=14695981039346656037ull;
  for(unsigned i=0;i<sizeof(g_ram);i++){h^=g_ram[i];h*=1099511628211ull;}
  return h;
}
const uint8_t *sm_simulation_ram(void) {return g_ram;}
int sm_samus_x(void) { return samus_x_pos; }
int sm_samus_y(void) { return samus_y_pos; }
int sm_room(void) { return room_ptr; }
int sm_area(void) { return area_index; }
void sm_set_assisted_walljump(int enabled) {sm_walljump_mode(enabled);}
int sm_assisted_walljump(void) {return sm_walljump_assisted();}
int sm_player_motion(int field) {
  switch(field){case 0:return samus_movement_type;case 1:return samus_pose;case 2:return samus_y_dir;case 3:return samus_health;default:return 0;}
}
int sm_fx_type(void) { return fx_type; }
int sm_water_y(void) {
  return (fx_type&15)==6 && !(fx_liquid_options&4) && !(fx_y_pos&0x8000) ? fx_y_pos : 32767;
}
int sm_heated_room(void) {
  // Actual heat palette pre-instruction, independent of suit/damage immunity.
  for(int i=0;i<8;++i)if(palettefx_ids[i] && palettefx_pre_instr[i]==0xe379)return 1;
  return fx_type==2 && !(lava_acid_y_pos&0x8000);
}
void sm_set_engine_weather(int enabled) { sm_scene_weather=enabled; }
typedef struct {const char *name;uint16_t area,station,room;} SmDestination;
static const SmDestination destinations[]={
  {"Crateria - Vaisseau / pluie",0,0,0x91f8},
  {"Crateria - Refuge interieur",0,1,0x93d5},
  {"Brinstar - Puits vert",1,8,0x9ad9},
  {"Norfair - Cavernes chaudes",2,17,0xa923},
  {"Norfair - Entree des profondeurs",2,10,0xb236},
  {"Maridia - Ouest",4,16,0xd1dd},
  {"Maridia - Est",4,19,0xd48e},
  {"Vaisseau fantome - Hall",3,16,0xca08},
};
int sm_teleport_count(void) {return sizeof(destinations)/sizeof(*destinations);}
const char *sm_teleport_name(int i) {return i>=0 && i<sm_teleport_count()?destinations[i].name:"";}
int sm_teleport_room(int i) {return i>=0 && i<sm_teleport_count()?destinations[i].room:0;}
int sm_teleport(int i) {
  sm_run_invalidate(1);
  if(!initialized || i<0 || i>=sm_teleport_count() || game_state!=8 ||
     sm_message_active() || queued_message_box_index || samus_health==0 ||
     coroutine_state_0 || coroutine_state_1 || coroutine_state_2 || coroutine_state_3 || coroutine_state_4)return 0;
  const SmDestination *d=&destinations[i];
  // Re-enter the native loading pipeline, with equipment and world events kept.
  // Never write SRAM or fabricate a room/door/coordinate combination.
  SaveExploredMapTilesToSaved();
  area_index=d->area;load_station_index=d->station;
  LoadMirrorOfExploredMapTiles();
  loading_game_state=5;
  game_state=6;
  sm_scene_parallax=0;
  sm_walljump_reset();sm_spacejump_reset();
  return 1;
}
int sm_camera_x(void) { return layer1_x_pos; }
int sm_camera_y(void) { return layer1_y_pos; }
/* The message coroutine temporarily owns BG3SC=0x58, then restores it.
 * message_box_index alone can remain set after closing. */
int sm_message_active(void) { return gameplay_BG3SC==0x58 && message_box_index!=0 && coroutine_state_3!=0; }
int sm_presentation_kind(void) {
  if(sm_credits_state(0))return 2;
  switch(game_state) {
    case 7:case 8:case 9:case 10:case 11:case 12:case 18:
    case 19:case 20:case 21:case 22:case 23:case 24:case 25:
    case 27:case 32:case 33:case 35:case 36:case 42:return 1;
    case 1:case 2:case 4:case 30:case 34:case 37:case 38:case 39:return 2;
    default:return 0;
  }
}
int sm_brightness(void) { return g_snes ? g_snes->ppu->brightness : 0; }
int sm_slots_copy_sram(uint8_t *out,int capacity){if(!initialized || !out || capacity<8192)return 0;memcpy(out,g_sram,8192);return 8192;}
int sm_save(void) {
  if(defer_sram_write)return 1;
  if (!initialized) return 0;
  char tmp[4100]; snprintf(tmp, sizeof(tmp), "%s.tmp", save_path);
  FILE *f = fopen(tmp, "wb"); if (!f) return 0;
  int ok = fwrite(g_sram, 1, 8192, f) == 8192;
  if (fclose(f)) ok = 0;
  return ok && rename(tmp, save_path) == 0 && sm_stats_save() && sm_runs_save() && sm_route_save();
}
void sm_shutdown(void) {
  if (!initialized) return;
  sm_route_close();
  sm_credits_reset();
  sm_soundtrack_reset();
  sm_save();
  ppu_free(g_snes->ppu == g_snes->my_ppu ? g_snes->snes_ppu : g_snes->my_ppu);
  snes_free(g_snes); g_snes = NULL;
  free(g_spc_player->dsp); free(g_spc_player); g_spc_player = NULL;
  initialized = 0;sm_seed_running(0);
  if(sm_slots_managed())sm_seed_clear();
}

int sm_test_gameover(void){
  if(!initialized || !strstr(save_path,"SMTests") || game_state!=8)return 0;
  game_state=kGameState_26_GameOverMenu;menu_index=0;
  screen_fade_delay=screen_fade_counter=1;return 1;
}
int sm_test_rendering(int enabled) {
  if(!initialized || !strstr(save_path,"SMTests"))return 0;
  g_snes->disableRender=!enabled;return 1;
}
int sm_test_combat_equipment(void) {
  if(!initialized || !strstr(save_path,"SMTests") || game_state!=8)return 0;
  sm_run_invalidate(1);
  collected_items|=0x1004;equipped_items|=0x1004;
  collected_beams|=0x1000;equipped_beams|=0x1000;
  samus_max_power_bombs=samus_power_bombs=10;
  return 1;
}
int sm_test_room(int room,int x,int y) {
  if(!initialized || !strstr(save_path,"SMTests") || game_state!=8)return 0;
  const uint16_t allowed[]={0x948c,0x96ba,0x975c,0x97b5,0x9e9f,0x9f11,0x9f64,0xd104,0xacb3,0xa66a,0xa5ed,0xddc4,0xdd58,0xacf0,0x9dc7};
  int found=0;for(unsigned i=0;i<sizeof(allowed)/sizeof(*allowed);i++)found|=room==allowed[i];
  if(!found)return 0;
  const RoomDefHeader *r=get_RoomDefHeader(room);
  if(x<32 || y<32 || x>=r->width*256-16 || y>=r->height*256-16)return 0;
  if(!sm_teleport(1))return 0;
  test_room_pending=room;test_room_x=x;test_room_y=y;test_room_door=0;return 1;
}
int sm_test_boss_connection(int index){
  if(!initialized || !strstr(save_path,"SMTests") || game_state!=8)return 0;
  int room,door,x,y;
  if(!sm_connections_test_destination(index,&room,&door,&x,&y) || !sm_teleport(1))return 0;
  test_room_pending=room;test_room_door=door;test_room_x=x;test_room_y=y;return 1;
}
int sm_test_area_connection(int index){
  if(!initialized || !strstr(save_path,"SMTests") || game_state!=8)return 0;
  int room,door,x,y;
  if(!sm_areas_test_destination(index,&room,&door,&x,&y) || !sm_teleport(1))return 0;
  test_room_pending=room;test_room_door=door;test_room_x=x;test_room_y=y;return 1;
}
int sm_test_awaken(void) {
  if(!initialized || !strstr(save_path,"SMTests") || game_state!=8)return 0;
  SetEventHappened(0);return 1;
}
void sm_test_load_room(void) {
  if(!test_room_pending)return;
  room_ptr=test_room_pending;test_room_pending=0;
  const RoomDefHeader *r=get_RoomDefHeader(room_ptr);
  area_index=r->area_index_;LoadMirrorOfExploredMapTiles();
  /* Use an actual incoming door. An outgoing door would make the native room
   * loader correctly load its neighbour, despite the requested test room. */
  const uint16_t pairs[][2]={{0x948c,0x8ad2},{0x96ba,0x8c76},{0x975c,0x8b62},{0x97b5,0x8b86},
    {0x9e9f,0x8ec2},{0x9f11,0x8eaa},{0x9f64,0x8ece},{0xd104,0x90c6},{0xacb3,0x97ce},{0xa66a,0x91f2},{0xa5ed,0x9216},{0xddc4,0xaa5c},{0xdd58,0xaac8},{0xacf0,0x95be},{0x9dc7,0x8e3e}};
  for(unsigned i=0;i<sizeof(pairs)/sizeof(*pairs);i++)if(room_ptr==pairs[i][0])door_def_ptr=pairs[i][1];
  if(test_room_door){door_def_ptr=test_room_door;test_room_door=0;}
  layer1_x_pos=bg1_x_offset=(test_room_x/256)*256;
  layer1_y_pos=bg1_y_offset=(test_room_y/256)*256;
  samus_x_pos=samus_prev_x_pos=test_room_x;samus_y_pos=samus_prev_y_pos=test_room_y;
}

int sm_test_all_equipment(void) {
  if(!initialized || !strstr(save_path,"SMTests") || game_state!=8 || sm_message_active())return 0;
  sm_run_invalidate(1);
  collected_items=equipped_items=0xf32f;
  collected_beams=0x100f;
  equipped_beams=0x100b; /* Spazer and Plasma are mutually exclusive. */
  samus_health=samus_max_health=1499;
  samus_missiles=samus_max_missiles=230;
  samus_super_missiles=samus_max_super_missiles=50;
  samus_power_bombs=samus_max_power_bombs=50;
  samus_reserve_health=samus_max_reserve_health=400;reserve_health_mode=1;
  UpdateBeamTilesAndPalette();InitializeHud();SaveToSram(0);return sm_save();
}
int sm_test_message(int index) {
  if(!initialized || !strstr(save_path,"SMTests") || game_state!=8 || sm_message_active() || index<1 || index>28)return 0;
  queued_message_box_index=index;return 1;
}
int sm_test_escape_timer(void) {
  if(!initialized || !strstr(save_path,"SMTests") || game_state!=8 || sm_message_active())return 0;
  // Ordinary rooms do not load countdown graphics. Use the original escape
  // transfer descriptors, without invoking a boss AI or changing world events.
  for(const uint8_t *entry=RomPtr_A6(0xc4cb);GET_WORD(entry);entry+=7)
    CopyToVramNow(GET_WORD(entry+5),entry[2]|entry[3]<<8|entry[4]<<16,GET_WORD(entry));
  timer_status=1;CallSomeSamusCode(0x0f);return 1;
}

/* Commit only in the new-file options screen, between complete native frames.
 * The source ROM is never written and no SRAM is copied from another profile. */
int sm_generation_commit(const SmSeedItem *items,int count,const char *fingerprint,const char *sram_path) {
  if(initialized && sm_slots_managed()) {
    if(!sm_generation_is_working() || !items || count!=100 || !fingerprint || !sram_path || strcmp(sram_path,save_path))return 0;
    int previous_start=sm_start_active_slot();
    if(!sm_start_activate(sm_slots_current(),1))return 0;
    if(!sm_seed_select_plan(items,count,fingerprint)){sm_start_activate(previous_start,previous_start>=0);return 0;}
    uint8_t *previous_ram=malloc(sizeof(g_ram));
    if(!previous_ram){sm_start_activate(previous_start,previous_start>=0);sm_seed_select_plan(0,0,0);return 0;}
    memcpy(previous_ram,g_ram,sizeof(g_ram));
    uint8_t previous[8192];memcpy(previous,g_sram,sizeof(previous));
    // NewSaveFile has already initialized the selected slot. Persist the normal
    // Effective start and native checksums immediately, before Start.
    sm_start_initialize_save();
    memset(map_tiles_explored,0,256);
    sm_seed_frame();
    uint16_t previous_loading=loading_game_state;loading_game_state=area_index==6?31:5;
    defer_sram_write=1;SaveToSram(selected_save_slot);defer_sram_write=0;loading_game_state=previous_loading;
    if(!sm_slots_action(4,selected_save_slot,0)){memcpy(g_ram,previous_ram,sizeof(g_ram));free(previous_ram);memcpy(g_sram,previous,sizeof(previous));sm_start_activate(previous_start,previous_start>=0);sm_seed_select_plan(0,0,0);return 0;}
    free(previous_ram);
    if(!sm_slots_commit(items,count,fingerprint))return 0;
    nonempty_save_slots|=1<<selected_save_slot;
    sm_generation_complete();sm_tracker_new_session();return 1;
  }
  if(!initialized || !sm_generation_is_working() || sm_seed_active() || !items || count!=100 ||
     !fingerprint || strlen(fingerprint)!=64 || !sram_path || strlen(sram_path)>=sizeof(save_path) ||
     !strstr(sram_path,fingerprint))return 0;
  // Check every address and PLM through the existing staging validation before reading ROM.
  sm_seed_running(0);
  int ok=sm_seed_stage(items,count,fingerprint);
  if(ok)ok=sm_seed_apply((uint8_t*)g_rom);
  if(!ok)sm_seed_clear();
  sm_seed_running(1);
  if(!ok)return 0;
  snprintf(save_path,sizeof(save_path),"%s",sram_path);sm_stats_open(save_path);sm_relic_escape_open(save_path);sm_runs_open(save_path);sm_route_open(save_path);
  sm_generation_complete();sm_tracker_new_session();
  return 1;
}

int sm_test_map_explored(int explored){
  if(!initialized || !strstr(save_path,"SMTests") || game_state!=8 || (explored!=0 && explored!=1))return 0;
  memset(map_tiles_explored,explored?255:0,256);
  memset(explored_map_tiles_saved,explored?255:0,2048);
  return 1;
}
int sm_test_credits_ending(void){
  if(!initialized || !strstr(save_path,"SMTests") || game_state!=8 || sm_message_active() || sm_credits_state(0))return 0;
  /* The native escape fade clears room HDMA/blending before cinematic setup. */
  game_state=38;screen_fade_delay=screen_fade_counter=1;
  return 1;
}

int sm_test_cinematic(int scene){
  if(scene<0||scene>5||scene==3||!initialized||!strstr(save_path,"SMTests")||sm_credits_state(0))return 0;
  // Same cleanup required by the real escape fade; test fixture only.
  DisableHdmaObjects();WaitUntilEndOfVblankAndClearHdma();DisableIrqInterrupts();
  fx_layer_blending_config_a=next_gameplay_CGWSEL=next_gameplay_CGADSUB=0;
  timer_status=power_bomb_explosion_status=0;
  layer1_x_pos=layer1_y_pos=0;
  music_timer=music_entry=music_queue_write_pos=music_queue_read_pos=sound_handler_downtime=0;
  memset(music_queue_track,0,16);memset(music_queue_delay,0,16);
  for(int i=656;i>=0;i-=2)*(uint16*)((uint8*)&cinematic_var5+i)=0;
  if(scene==4){game_state=1;cinematic_function=0x9b68;return 1;}
  if(scene==5){
    // Isolated managed-menu fixture: A/C vanilla, B pending randomized.
    // No callback can mutate profiles; all three entries use the normal renderer.
    sm_slots_enable(0,0);
    for(int i=0;i<3;i++)sm_slots_set(i,i==1,i!=1,0,0,"");
    game_state=4;menu_index=game_options_screen_index=0;reg_BGMODE=1;return 1;
  }
  if(scene==2)loading_game_state=kLoadingGameState_22_EscapingCeres;
  game_state=scene==2?34:30;cinematic_function=scene==1?0xbca0:scene==2?0xc11b:0xa395;
  return 1;
}

int sm_test_playtest(int action,int value){
  if(!initialized || !strstr(save_path,"SMTests") || game_state!=8 || sm_message_active())return -1;
  switch(action){
    case 0:sm_stats_finish();return sm_save();
    case 1:SaveToSram(selected_save_slot);return sm_save();
    case 2:return sm_run_enemy_damage(value);
    case 3:return sm_run_contact_damage(value);
    case 4:sm_visual_sprite(samus_x_pos,samus_y_pos,142);return 1;
    case 5:return sm_slots_action(2,selected_save_slot,value);
    case 6:return sm_slots_action(3,value,0);
    case 7:sm_run_enemy_spawn(cur_enemy_index);return 1;
    case 8:if(value<0 || value>2 || !sm_slots_select(value))return 0;selected_save_slot=value;return 1;
    case 10:sm_run_new_game();return 1;
    default:return -1;
  }
}
