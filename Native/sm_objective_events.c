#include "sm_objective_events.h"
#include "sm_seed.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include "enemy_types.h"
#include "sm_rtl.h"
#include <string.h>
#include "sm_objective_events.inc"
/* Independent of original map bytes 0..326 and ZME1 at $150..$154. Native
 * SaveToSram/LoadFromSram checksum and copy the complete field unchanged. */
#define EVENT_SAVE_OFFSET 0x180
static uint8_t *event_bits(void){return compressed_map_data+EVENT_SAVE_OFFSET+4;}
int sm_objective_event(unsigned event){
  if(!sm_seed_active() || event>OBJECTIVE_EVENT_LAST)return 0;
  if(event<128)return !!(events_that_happened[event>>3]&(1<<(event&7)));
  if(memcmp(compressed_map_data+EVENT_SAVE_OFFSET,"ZOE1",4))return 0;
  event-=128;return !!(event_bits()[event>>3]&(1<<(event&7)));
}
void sm_objective_event_mark(unsigned event){
  if(!sm_seed_active() || event<128 || event>OBJECTIVE_EVENT_LAST)return;
  if(memcmp(compressed_map_data+EVENT_SAVE_OFFSET,"ZOE1",4)){
    memset(event_bits(),0,OBJECTIVE_EVENT_BYTES);
    memcpy(compressed_map_data+EVENT_SAVE_OFFSET,"ZOE1",4);
  }
  event-=128;event_bits()[event>>3]|=1<<(event&7);
}
uint16_t sm_objective_enemy_properties(unsigned bank,unsigned population,uint16_t original){
  if(!sm_seed_active() || bank!=0xa1)return original;
  for(unsigned i=0;i<sizeof(objective_enemies)/sizeof(objective_enemies[0]);i++)
    if(objective_enemies[i].population==population)return objective_enemies[i].properties;
  return original;
}
int sm_objective_enemy_count(int type,int field){
  if(!sm_seed_active() || type<0 || type>=6 || (field!=0 && field!=1))return -1;
  if(field)return objective_enemy_totals[type];
  int n=0;
  for(unsigned i=0;i<sizeof(objective_enemies)/sizeof(objective_enemies[0]);i++)
    if(objective_enemies[i].type==type)n+=sm_objective_event(objective_enemies[i].event);
  return n;
}
void sm_objective_enemy_death(unsigned index){
  if(!sm_seed_active() || index>=2048 || index%64)return;
  EnemyData *e=gEnemyData(index);
  if(!(e->extra_properties&0x4000))return;
  unsigned id=(e->extra_properties&0x3ff8)>>3;
  if(id>=sizeof(objective_enemies)/sizeof(objective_enemies[0]) || sm_objective_event(objective_enemies[id].event))return;
  unsigned room_event=objective_enemies[id].room_event,type=objective_enemies[id].type;
  sm_objective_event_mark(objective_enemies[id].event);
  int room_done=1;
  for(unsigned i=0;i<sizeof(objective_enemies)/sizeof(objective_enemies[0]);i++)
    if(objective_enemies[i].room_event==room_event && !sm_objective_event(objective_enemies[i].event)){room_done=0;break;}
  if(room_done)sm_objective_event_mark(room_event);
  if(sm_objective_enemy_count(type,0)>=objective_enemy_totals[type])sm_objective_event_mark(objective_enemy_all_events[type]);
}
/* Wrap the actual dispatcher: the original call still runs once with the
 * original argument, and unrelated enemy AIs never touch objective events. */
void sm_original_CallEnemyAi(uint32 ea);
void CallEnemyAi(uint32 ea){
  unsigned index=cur_enemy_index,enemy=index<2048?gEnemyData(index)->enemy_ptr:0;
  sm_original_CallEnemyAi(ea);
  if(!sm_seed_active() || index>=2048 || index%64)return;
  for(unsigned i=0;i<sizeof(objective_special_ai)/sizeof(objective_special_ai[0]);i++){
    if(enemy!=objective_special_ai[i].enemy || ea!=objective_special_ai[i].target)continue;
    switch(objective_special_ai[i].kind){
      case 0:if(room_ptr==0xd104)sm_objective_event_mark(OBJ_FISH_TICKLED);break;
      case 1:if(!gEnemyData(0)->health)sm_objective_event_mark(OBJ_ORANGE_GEEMER);break;
      case 2:if(!gEnemyData(index)->health)sm_objective_event_mark(OBJ_SHAK_DEAD);break;
      case 3:if(room_ptr==0xacb3 && !gEnemyData(index)->health)sm_objective_event_mark(OBJ_KING_CAC);break;
    }
    break;
  }
}
void sm_objective_events_frame(void){
  if(!sm_seed_active() || game_state!=8 || area_index!=1)return;
  if(map_tiles_explored[0x828-0x7f7]&0x10)sm_objective_event_mark(OBJ_ETECOONS);
  if(map_tiles_explored[0x82c-0x7f7]&0x20)sm_objective_event_mark(OBJ_DACHORA);
}
