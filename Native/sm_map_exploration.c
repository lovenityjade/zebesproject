#include "sm_map_exploration.h"
#include "sm_seed.h"
#include "sm_minimizer.h"
#include "sm_tourian.h"
#include "sm_escape.h"
#include "sm_areas.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
#include "sm_map_exploration.inc"

/* Brinstar Fireflea: VARIA adds a counted room tile omitted by the original
 * map. Its original tile $25 is identical; keep the native map palette. */
static uint8_t *captured;
static uint8_t original_fireflea[2];
void sm_map_exploration_capture(uint8_t *rom){
  captured=rom;memcpy(original_fireflea,rom+0x1a820e,2);
}
void sm_map_exploration_apply(uint8_t *rom,int randomized){
  if(rom!=captured)return;
  memcpy(rom+0x1a820e,original_fireflea,2);
  if(randomized){rom[0x1a820e]=0x25;rom[0x1a820f]=0xcc;}
}
void sm_map_exploration_mark(unsigned byte,unsigned mask){
  if(byte>=256 || mask>255)return;
  int fresh=!(map_tiles_explored[byte]&mask);
  map_tiles_explored[byte]|=mask;
  if(!fresh || !sm_seed_active())return;
  for(unsigned i=0;i<sizeof(exploration_slopes)/sizeof(exploration_slopes[0]);i++)
    if(area_index==exploration_slopes[i].area && byte==exploration_slopes[i].byte && mask==exploration_slopes[i].mask){
      if(byte>=4)sm_map_exploration_mark(byte-4,mask);
      return;
    }
}
static void reveal_portal(int index,int matching){
  if(!exploration_portals[index].relocated)return;
  unsigned area=exploration_portals[index].area;
  ((uint8_t*)explored_map_tiles_saved)[area*256+exploration_portals[index].byte]|=exploration_portals[index].mask;
  /* Exact source writeExploreMapAsm mirror condition, after arrival selects
   * the destination's physical area. The departure mirror was already saved. */
  if(area==(sm_minimizer_active()?sm_minimizer_room_area(matching):exploration_portals[matching].room_area))LoadMirrorOfExploredMapTiles();
}
void sm_map_exploration_portal(int source,int destination){
  int mixed=sm_minimizer_active(),count=mixed?40:32;
  if(source<0 || source>=count || destination<0 || destination>=count ||
     (mixed?sm_minimizer_destination(source):sm_areas_destination(source))!=destination)return;
  if(source<32)reveal_portal(source,destination);
  if(source!=destination && destination<32)reveal_portal(destination,source);
}
int sm_map_exploration_region(void){
  int layout=sm_minimizer_active() || sm_areas_destination(0)>=0;
  static const uint8_t fallback[]={1,2,6,4,9,11,0};
  for(unsigned i=0;i<sizeof(exploration_rooms)/sizeof(exploration_rooms[0]);i++)
    if(exploration_rooms[i].room==room_ptr)return exploration_rooms[i].owner[layout];
  return area_index<7?fallback[area_index]:0;
}
int sm_map_exploration_total(int area_layout,int region){
  if(area_layout<0 || area_layout>1 || region < -1 || region>=12)return -1;
  if(region>=0)return exploration_totals[area_layout][region];
  int total=0;for(int r=0;r<12;r++)total+=exploration_totals[area_layout][r];
  return total;
}
int sm_map_exploration_value(int region,int field){
  if(!sm_seed_active() || region < -1 || region>=12 || (field!=0 && field!=1))return -1;
  int layout=sm_minimizer_active() || sm_areas_destination(0)>=0,total=0;
  if(field==1 && sm_minimizer_active()){
    if(region>=0)return sm_minimizer_map_total(-1,region)-sm_escape_map_offset(-1,region);
    for(int r=0;r<12;r++)total+=sm_minimizer_map_total(-1,r);return total-sm_escape_map_offset(-1,-1);
  }
  if(field==1)return sm_map_exploration_total(layout,region)-sm_escape_map_offset(-1,region)-(!layout && (region==-1 || region==1) && sm_tourian_fast()?1:0);
  for(unsigned i=0;i<sizeof(exploration_tiles)/sizeof(exploration_tiles[0]);i++){
    unsigned r=exploration_tiles[i].owner[layout],a=exploration_tiles[i].area;
    if(!sm_minimizer_map_tile(r,a,exploration_tiles[i].byte,exploration_tiles[i].mask))continue;
    if(!sm_escape_map_tile(a,exploration_tiles[i].byte,exploration_tiles[i].mask))continue;
    if(!exploration_totals[layout][r] || (region>=0 && r!=region))continue;
    if(!layout && sm_tourian_fast() && a==0 && exploration_tiles[i].byte==34 && exploration_tiles[i].mask==64)continue;
    const uint8_t *bits=a==area_index?map_tiles_explored:(uint8_t*)explored_map_tiles_saved+a*256;
    total+=!!(bits[exploration_tiles[i].byte]&exploration_tiles[i].mask);
  }
  return total;
}
/* The original compressed map occupies bytes 0..326 of its 1280-byte save
 * field. East Tunnel Top Right's display byte was never in that packing
 * table. Version this small extension without changing any original offset,
 * slot checksum or historical map byte. Counts need no independent SRAM. */
void sm_map_exploration_pack(void){
  if(!sm_seed_active())return;
  memcpy(compressed_map_data+0x150,"ZME1",4);
  compressed_map_data[0x154]=((uint8_t*)explored_map_tiles_saved)[256+201]&0x80;
}
void sm_map_exploration_unpack(void){
  if(!sm_seed_active() || memcmp(compressed_map_data+0x150,"ZME1",4))return;
  ((uint8_t*)explored_map_tiles_saved)[256+201]|=compressed_map_data[0x154]&0x80;
}
