#include "sm_mirror.h"
#include "sm_start.h"
#include "sm_objective_events.h"
#include "sm_world_data.h"
#include "sm_seed.h"
#include "sm_seed_rules.h"
#include "sm_tracker.h"
#include "ida_types.h"
#include "sm_rtl.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
static const uint32_t addresses[]={
#include "../Unreal/Source/SMUnreal/SMItemAddresses.inl"
};
static SmSeedItem original_items[100];
static uint32_t item_address(int index){return sm_mirror_item_address(index,addresses[index]);}
static const uint8_t can_hide[100]={
#include "sm_can_hide.inc"
};
static int visibility_allowed(int index,uint16_t plm){
  int original=(original_items[index].plm-0xeed7)/84, next=(plm-0xeed7)/84;
  return original==next || (original==0 && next==2 && can_hide[index]);
}
static uint16_t placements[100],item_kinds[100];
static char fingerprint[65];
static int staged,running;
int sm_seed_stage(const SmSeedItem *items,int count,const char *hash) {
  if(running||!items||count!=100||!hash||strlen(hash)!=64)return 0;
  for(int i=0;i<64;i++)if(!((hash[i]>='0'&&hash[i]<='9')||(hash[i]>='a'&&hash[i]<='f')))return 0;
  uint16_t next[100]={0},kinds[100]={0};
  for(int i=0;i<count;i++) {
    int found=-1;
    for(int j=0;j<100;j++)if(addresses[j]==items[i].address){found=j;break;}
    if(found<0||next[found]||items[i].plm<0xeed7||items[i].plm>0xefcf||(items[i].plm-0xeed7)%4)return 0;
    if(items[i].kind>2 || (items[i].kind && (items[i].plm-0xeed7)%84!=80))return 0;
    next[found]=items[i].plm;kinds[found]=items[i].kind;
  }
  memcpy(item_kinds,kinds,sizeof(kinds));memcpy(placements,next,sizeof(next));memcpy(fingerprint,hash,65);staged=1;return 1;
}
int sm_seed_clear(void) {if(running)return 0;staged=0;memset(item_kinds,0,sizeof(item_kinds));fingerprint[0]=0;return 1;}
int sm_seed_active(void) {return staged;}
const char *sm_seed_fingerprint(void) {return fingerprint;}
void sm_seed_running(int enabled) {running=enabled;}
int sm_seed_apply(uint8_t *rom) {
  if(!staged)return 1;
  for(int i=0;i<100;i++) {
    uint16_t old=original_items[i].plm;
    if(old<0xeed7||old>0xefcf||(old-0xeed7)%4)return 0;
    // Preserve visible/Chozo/hidden behavior and the original location ID.
    if(!visibility_allowed(i,placements[i]))return 0;
  }
  sm_world_data_apply(rom,1);
  for(int i=0;i<100;i++){uint32_t a=item_address(i);rom[a]=placements[i]&255;rom[a+1]=placements[i]>>8;}
  return 1;
}
int sm_seed_rom_item(int index) {return running && index>=0&&index<100?GET_WORD(g_rom+item_address(index)):-1;}
void sm_seed_frame(void) {
  if(!staged)return;
  sm_objective_events_frame();
  // VARIA start flags: Red Tower elevator and Blue Brinstar green door.
  opened_door_bit_array[0x10>>3]|=1<<(0x10&7);
  int start=sm_start_spawn(sm_start_active_slot());
  if(start<=0 || start==0xfffe)opened_door_bit_array[0x32>>3]|=1<<(0x32&7);
  // Native equivalent of wake_zebes door ASM at 83:8EB4 (door 8EAA).
  if(door_def_ptr==0x8eaa && game_state>=9 && game_state<=11)events_that_happened[0]|=1;
}
int sm_seed_morph_collected(void) {
  return staged ? !!(item_bit_array[0x1a>>3]&(1<<(0x1a&7))) : !!(collected_items&4);
}
uint16_t sm_seed_equipment_mask(const uint8_t *operand,uint16_t plm) {
  /* Native equivalent of VARIA patches/common/src/vanilla_bugfixes.asm,
   * 84:E8CE and 84:EE02. These two unused vanilla Morph variants contain
   * Spring Ball's bit despite their correct Morph graphics/message. Keep
   * the original ROM and all unrelated equipment grants unchanged. */
  if(staged && plm<80 && !(plm&1) &&
     (plm_header_ptr[plm>>1]==0xef77 || plm_header_ptr[plm>>1]==0xefcb))return 4;
  return GET_WORD(operand);
}
void sm_seed_room_items(void) {
  if(!staged||room_ptr!=0x9e9f)return;
  // The awakened room state omits Morph's original location. Reuse its real
  // entry, graphics, item PLM and saved bit instead of inserting custom ASM.
  for(int i=0;i<40;i++)if(plm_header_ptr[i]>=0xeed7&&plm_header_ptr[i]<=0xefcf&&plm_room_arguments[i]==0x1a)return;
  SpawnRoomPLM(0x86de);
}

int sm_seed_location_collected(int index) {
  if(!running||index<0||index>=100)return 0;
  int bit=GET_WORD(g_rom+item_address(index)+4)&255;
  return !!(item_bit_array[bit>>3]&(1<<(bit&7)));
}
int sm_test_seed_room(int awakened) {
  if(!staged||!running||game_state!=8||coroutine_state_0||coroutine_state_1||coroutine_state_2||coroutine_state_3||coroutine_state_4)return 0;
  events_that_happened[0]=(events_that_happened[0]&~1)|!!awakened;
  SaveExploredMapTilesToSaved();area_index=1;load_station_index=9;LoadMirrorOfExploredMapTiles();
  loading_game_state=5;game_state=6;return 1;
}
int sm_test_seed_pickup_position(void) {
  if(!staged||!running||game_state!=8||room_ptr!=0x9e9f)return 0;
  // Test fixture starts beside the real Morph location; collection still uses
  // normal movement, collision, PLM instructions, message and inventory code.
  samus_x_pos=samus_prev_x_pos=69*16+40;
  samus_y_pos=samus_prev_y_pos=41*16+8;
  samus_x_subpos=samus_y_subpos=0;
  return 1;
}
int sm_seed_inventory(int field) {
  switch(field) {
    case 0:return samus_max_health;
    case 1:return samus_max_missiles;
    case 2:return samus_max_super_missiles;
    case 3:return samus_max_power_bombs;
    case 4:return collected_items;
    case 5:return collected_beams;
    case 6:return samus_max_reserve_health;
    default:return 0;
  }
}
static uint8_t complete_maps[7][256];
const uint8_t *sm_seed_map_data(unsigned area){
  if(area>=7)area=0;
  return sm_map_fully_known()?complete_maps[area]:RomPtr_82(GET_WORD(RomFixedPtr(0x829717)+area*2));
}
void sm_seed_refresh_maps(void) {
  // The authored tilemaps contain the secret rooms omitted by map stations.
  // Keep their native two-page bit layout and exclude the blank tile (0x1f).
  memset(complete_maps,0,sizeof(complete_maps));
  for(unsigned area=0;area<7;area++){
    const uint8_t *p=RomFixedPtr(0x82964a)+area*3;
    const uint8_t *map=RomPtr(p[0]|p[1]<<8|p[2]<<16);
    for(unsigned i=0;i<2048;i++)if((GET_WORD(map+i*2)&0x3ff)!=0x1f)
      complete_maps[area][i>>3]|=0x80>>(i&7);
  }
}
void sm_seed_capture_original(void) {
  sm_world_data_capture((uint8_t*)g_rom);
  sm_seed_refresh_maps();
  for(int i=0;i<100;i++)original_items[i]=(SmSeedItem){addresses[i],GET_WORD(g_rom+addresses[i])};
}
int sm_seed_select_plan(const SmSeedItem *items,int count,const char *hash) {
  // Validate the complete plan and visibility before changing any ROM byte.
  uint16_t old[100],oldkinds[100];char oldhash[65];int oldstaged=staged;
  memcpy(oldkinds,item_kinds,sizeof(oldkinds));memcpy(old,placements,sizeof(old));memcpy(oldhash,fingerprint,65);
  running=0;
  int ok=items?sm_seed_stage(items,count,hash):sm_seed_clear();
  if(ok && items)ok=sm_seed_apply((uint8_t*)g_rom);
  if(ok && !items)sm_world_data_apply((uint8_t*)g_rom,0);
  if(ok && !items)for(int i=0;i<100;i++){
    uint32_t a=original_items[i].address;uint16_t p=original_items[i].plm;
    ((uint8_t*)g_rom)[a]=p;((uint8_t*)g_rom)[a+1]=p>>8;
  }
  if(!ok){memcpy(item_kinds,oldkinds,sizeof(oldkinds));memcpy(placements,old,sizeof(old));memcpy(fingerprint,oldhash,65);staged=oldstaged;}
  running=1;return ok;
}

int sm_seed_validate_plan(const SmSeedItem *items,int count,const char *hash){
  uint16_t old[100],oldkinds[100];char oldhash[65];int oldstaged=staged,oldrunning=running;
  memcpy(oldkinds,item_kinds,sizeof(oldkinds));memcpy(old,placements,sizeof(old));memcpy(oldhash,fingerprint,65);running=0;
  int ok=sm_seed_stage(items,count,hash);
  if(ok)for(int i=0;i<100;i++)if(!visibility_allowed(i,placements[i])){ok=0;break;}
  memcpy(item_kinds,oldkinds,sizeof(oldkinds));memcpy(placements,old,sizeof(old));memcpy(fingerprint,oldhash,65);staged=oldstaged;running=oldrunning;return ok;
}

int sm_map_fully_known(void){return sm_seed_active()?!sm_seed_rule(SM_SEED_HIDDEN_MAP):sm_tracker_reveals_map();}

int sm_seed_item_kind(int index){return staged && index>=0 && index<100?item_kinds[index]:0;}
int sm_seed_plm_kind(uint16_t plm){
  if(!staged || plm>=80 || plm&1)return 0;
  unsigned h=plm_header_ptr[plm>>1];if(h<0xeed7 || h>0xefcf)return 0;
  unsigned bit=plm_room_arguments[plm>>1]&255;
  for(int i=0;i<100;i++)if((GET_WORD(g_rom+item_address(i)+4)&255)==bit)return item_kinds[i];
  return 0;
}
