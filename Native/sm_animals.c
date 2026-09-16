#include "sm_animals.h"
#include "sm_seed.h"
#include "sm_run_stats.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
#include "sm_animals_data.inc"
#include "sm_animals_rom_assets.inc"
#include "sm_room_rom_loader.h"
void sm_animals_load_assets(void){sm_room_rom_load(animals_bytes,sizeof(animals_bytes));}
void RoomCode_SetPauseCodeForDraygon(void);
static uint8_t original[ANIMALS_DATA_SIZE],configurations[4];
static uint8_t *captured;
static int selected=-1,applied;
const char *sm_animals_catalog_sha256(void){return animals_catalog;}
int sm_animals_configure(int slot,int mode,const char *catalog){
  if(slot<0 || slot>=4 || mode<0 || mode>10 || (mode && (!catalog || strcmp(catalog,animals_catalog))))return 0;
  configurations[slot]=mode;return 1;
}
void sm_animals_reset(void){memset(configurations,0,sizeof(configurations));selected=-1;applied=0;}
void sm_animals_activate(int slot){selected=slot>=0 && slot<4?slot:-1;}
int sm_animals_state(void){return sm_seed_active()?applied:0;}
int sm_animals_hostile(void){int m=sm_animals_state();return m==1 || m==2;}
int sm_animals_escape_event(void){return sm_animals_state()?14:15;}
void sm_animals_capture(uint8_t *rom){
  captured=rom;
  for(unsigned i=0;i<sizeof(animals_spans)/sizeof(*animals_spans);i++)
    memcpy(original+animals_spans[i].offset,rom+animals_spans[i].address,animals_spans[i].size);
}
void sm_animals_restore(uint8_t *rom){
  if(rom!=captured)return;
  for(unsigned i=0;i<sizeof(animals_spans)/sizeof(*animals_spans);i++)
    memcpy(rom+animals_spans[i].address,original+animals_spans[i].offset,animals_spans[i].size);
}
void sm_animals_apply(uint8_t *rom,int randomized){
  if(rom!=captured)return;
  applied=randomized && selected>=0?configurations[selected]:0;
  for(unsigned i=animals_variants[applied].first;i<animals_variants[applied].first+animals_variants[applied].count;i++)
    memcpy(rom+animals_spans[i].address,animals_bytes+animals_spans[i].offset,animals_spans[i].size);
}
static void word(unsigned address,uint16_t value){g_ram[address]=value;g_ram[address+1]=value>>8;}
int sm_animals_room(uint32_t address){
  int mode=sm_animals_state();
  if(!(mode==3 || mode==4 || mode==8 || mode==9 || mode==10))return 0;
  if(address==0x8ff006){
    if(mode!=4){word(0xd829,0);word(0xd82b,0);}
    door_list_pointer=0xf000;return 1; /* Original $91BB is a null routine. */
  }
  if(address!=0x8ff01c || mode==4)return 0;
  if(events_that_happened[1]&0x40){
    switch(mode){
      case 3:
        door_list_pointer=0xf06f;word(0x1cd5,0xffff);
        word(0x101be,0x80ae);word(0x101fe,0x80ce);word(0x1023e,0x88ce);word(0x1027e,0x88ae);
        g_ram[0x166c2]=g_ram[0x166e2]=g_ram[0x16702]=g_ram[0x16722]=0;break;
      case 8:word(0xd8bc,0);door_list_pointer=0xf038;break;
      case 9:word(0xd8c0,0);door_list_pointer=0xf03b;break;
      case 10:
        word(0xd8bb,0);door_list_pointer=0xf05e;word(0x1cd5,0xaaaa);
        word(0x100de,0x80ae);word(0x100fe,0x80ce);word(0x1011e,0x88ce);word(0x1013e,0x88ae);break;
    }
  }
  if(mode==3)RoomCode_SetPauseCodeForDraygon();
  return 1; /* $91F7 and $C8D0 are null routines. */
}
int sm_animals_door(uint32_t address){
  int mode=sm_animals_state();
  if(address!=0x8fff20 || (mode!=5 && mode!=7))return 0;
  if(events_that_happened[1]&0x40){
    if(mode==7)word(0x946,0x20); /* BCD seconds, clear adjacent timer minutes byte as source STA does. */
    else {sm_stats_finish();game_state=38;}
  }
  return 1;
}
