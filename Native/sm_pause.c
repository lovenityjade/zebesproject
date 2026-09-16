#include "sm_bridge.h"
#include "sm_seed.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include "sm_rtl.h"
#include "sm_cpu_infra.h"
#include <string.h>
/* ROM-authored map/wireframe graphics, decoded without mutating the PPU. */
static uint8_t map_image[512*256*4],wire_image[64*136*4],atlas_image[128*256*4];
static void pause_tile(uint8_t *out,int pitch,uint16_t tile,int transparent) {
  const uint8_t *source=RomFixedPtr(0xb68000)+(tile&1023)*32;
  for(int y=0;y<8;y++)for(int x=0;x<8;x++) {
    int row=(tile&0x8000)?7-y:y,bit=(tile&0x4000)?x:7-x;
    int ci=((source[row*2]>>bit)&1)|(((source[row*2+1]>>bit)&1)<<1)|
      (((source[16+row*2]>>bit)&1)<<2)|(((source[17+row*2]>>bit)&1)<<3);
    uint16_t c=g_snes->ppu->cgram[((tile>>10)&7)*16+ci];
    uint8_t *p=out+y*pitch+x*4;
    p[0]=g_snes->ppu->brightnessMult[(c>>10)&31];
    p[1]=g_snes->ppu->brightnessMult[(c>>5)&31];p[2]=g_snes->ppu->brightnessMult[c&31];
    p[3]=transparent&&(!ci || !(p[0]|p[1]|p[2]))?0:255;
  }
}
const uint8_t *sm_pause_map(void) {
  memset(map_image,0,sizeof(map_image));
  if(game_state!=15 || area_index>=7)return map_image;
  const uint8_t *ptr=RomFixedPtr(0x82964a)+area_index*3;
  const uint16_t *map=(const uint16_t*)RomPtr(ptr[0]|ptr[1]<<8|ptr[2]<<16);
  const uint8_t *known=sm_seed_map_data(area_index);
  for(int y=0;y<32;y++)for(int x=0;x<64;x++) {
    int index=(x&31)+y*32+(x&32)*32,bit=((x&31)>>3)+4*((x&32)+y),mask=0x80>>(x&7);
    uint16_t tile=31;
    if(map_tiles_explored[bit]&mask)tile=map[index]&~0x400;
    else if((sm_map_fully_known() || map_station_byte_array[area_index]) && (known[bit]&mask))tile=map[index];
    pause_tile(map_image+(y*8*512+x*8)*4,512*4,tile,1);
  }
  return map_image;
}
const uint8_t *sm_pause_wire(void) {
  memset(wire_image,0,sizeof(wire_image));
  if(game_state!=15)return wire_image;
  int i=0;
  for(;i<4;i++)if(GET_WORD(RomFixedPtr(0x82b257)+i*2)==(equipped_items&0x101))break;
  if(i==4)return wire_image;
  const uint16_t *tiles=(const uint16_t*)RomPtr_82(GET_WORD(RomFixedPtr(0x82b25f)+i*2));
  for(int y=0;y<17;y++)for(int x=0;x<8;x++)pause_tile(wire_image+(y*8*64+x*8)*4,64*4,tiles[y*8+x],1);
  return wire_image;
}
const uint8_t *sm_pause_atlas(void) {
  if(game_state!=15)return atlas_image;
  for(int i=0;i<512;i++)pause_tile(atlas_image+((i/16)*8*128+(i%16)*8)*4,128*4,i|0x800,1);
  return atlas_image;
}
int sm_pause_data(int field) {
  switch(field) {
    case 0:return collected_items;case 1:return equipped_items;case 2:return collected_beams;case 3:return equipped_beams;
    case 4:return reserve_health_mode;case 5:return samus_reserve_health;case 6:return samus_max_reserve_health;
    case 7:return room_x_coordinate_on_map+(samus_x_pos>>8);
    case 8:return room_y_coordinate_on_map+(samus_y_pos>>8)+1;
    case 9:return samus_health;case 10:return samus_max_health;
    case 11:return menu_index;case 12:return pause_screen_mode;
    default:return 0;
  }
}
int sm_pause_action(int action,int index) {
  if(game_state!=15 || (menu_index!=0 && menu_index!=1))return 0;
  if(action==1) {
    if(!samus_max_reserve_health)return 0;
    reserve_health_mode=reserve_health_mode==1?2:1;
    if(reserve_health_mode==2)EquipmentScreenHudReserveAutoTilemap_Off();
    return 1;
  }
  if(action==2) {
    if(reserve_health_mode!=2 || !samus_reserve_health || samus_health>=samus_max_health)return 0;
    unsigned amount=samus_max_health-samus_health;if(amount>samus_reserve_health)amount=samus_reserve_health;
    samus_health+=amount;samus_reserve_health-=amount;return 1;
  }
  /* Native item bit tables and mutual exclusion rules, with the same acquired
   * checks as the equipment screen. The normal unpause applies the palettes. */
  static const uint16_t bits[]={0x1000,2,1,4,8,1,0x20,4,0x1000,2,8,0x100,0x200,0x2000};
  if(action!=0 || index<0 || index>=14)return 0;
  uint16_t bit=bits[index];
  if(index<5) {
    if(hyper_beam_flag || !(collected_beams&bit))return 0;
    equipped_beams^=bit;
    if((equipped_beams&12)==12)equipped_beams&=~(bit==4?8:4);
  } else {
    if(!(collected_items&bit))return 0;
    equipped_items^=bit;
  }
  return 1;
}
