#include "sm_bridge.h"
#include "ida_types.h"
#include "variables.h"
#include <string.h>

/* Authored visual overrides live outside emulated RAM and SRAM. They never
 * replace collision/BTS/PLM data, ROM tiles, or the game's random stream. */
#define DECOR_CAPACITY 131072
static struct DecorCell {
  uint16_t room,state,tile;int16_t x,y;
  uint8_t layer,tileset,used;
  int expected;
} decor[DECOR_CAPACITY];
static uint8_t decor_states[65536];
static unsigned decor_count;
static unsigned decor_hash(int room,int state,int layer,int x,int y) {
  return ((unsigned)room*31337u^(unsigned)state*83492791u^(unsigned)x*73856093u^
          (unsigned)y*19349663u^(unsigned)layer*2654435761u)&(DECOR_CAPACITY-1);
}
void sm_decor_clear(void) {memset(decor,0,sizeof(decor));memset(decor_states,0,sizeof(decor_states));decor_count=0;}
int sm_decor_set(int room,int state,int tileset,int layer,int x,int y,int tile,int expected) {
  if(room<0x8000||room>0xffff||state<0x8000||state>0xffff||tileset<0||tileset>31||
     layer<0||layer>1||x<-128||x>511||y<-128||y>511||tile<0||tile>4095||expected<-1||expected>65535)return 0;
  unsigned slot=decor_hash(room,state,layer,x,y);
  for(unsigned n=0;n<DECOR_CAPACITY;n++,slot=(slot+1)&(DECOR_CAPACITY-1)) {
    struct DecorCell *c=&decor[slot];
    if(!c->used) {
      if(decor_count>=100000)return 0;
      ++decor_count;
    } else if(c->room!=room||c->state!=state||c->layer!=layer||c->x!=x||c->y!=y)continue;
    *c=(struct DecorCell){room,state,tile,x,y,layer,tileset,1,expected};
    decor_states[state]=1;return 1;
  }
  return 0;
}
int sm_decor_room_active(void) {return decor_states[roomdefroomstate_ptr]!=0;}
int sm_decor_get(int layer,int x,int y,uint16_t *tile) {
  if(!sm_decor_room_active())return 0;
  unsigned slot=decor_hash(room_ptr,roomdefroomstate_ptr,layer,x,y);
  for(unsigned n=0;n<DECOR_CAPACITY;n++,slot=(slot+1)&(DECOR_CAPACITY-1)) {
    const struct DecorCell *c=&decor[slot];if(!c->used)return 0;
    if(c->room!=room_ptr||c->state!=roomdefroomstate_ptr||c->layer!=layer||c->x!=x||c->y!=y)continue;
    if(c->tileset!=get_RoomDefRoomstate(roomdefroomstate_ptr)->graphics_set)return 0;
    if(c->expected>=0 && x>=0&&y>=0&&x<room_width_in_blocks&&y<room_height_in_blocks) {
      int address=0x10002+2*(y*room_width_in_blocks+x)+(layer?0x9600:0);
      if(address<0 || address+1>=0x20000 || GET_WORD(g_ram+address)!=c->expected)return 0;
    }
    *tile=c->tile;return 1;
  }
  return 0;
}
