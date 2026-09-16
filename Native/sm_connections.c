#include "sm_connections.h"
#include "sm_seed.h"
#include "sm_minimizer.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
/* Bank-local upstream routines are global definitions without cross-bank
 * declarations in funcs.h; retain the pristine source headers. */
void CallDoorDefSetupCode(uint32 ea);
void Kraid_FadeInBg_LoadBg3Tiles1of4(void);
void Kraid_FadeInBg_LoadBg3Tiles2of4(void);
void Kraid_FadeInBg_LoadBg3Tiles3of4(void);
void Kraid_FadeInBg_LoadBg3Tiles4of4(void);
#include "sm_connections.inc"
typedef struct {uint8_t targets[8],enabled;} Routing;
static Routing configurations[4],applied;
static int selected=-1,cancel_spark;
static uint8_t original_doors[8][12],original_cre[8],original_songs[8][6],*captured;
const char *sm_connections_catalog_sha256(void){return connections_catalog;}
int sm_connections_destination(int index){return sm_seed_active() && applied.enabled && index>=0 && index<8?applied.targets[index]:-1;}
int sm_connections_configure(int slot,const uint8_t *destinations,int count,const char *catalog){
  if(slot<0 || slot>=4 || (count!=0 && count!=8) || !catalog || strcmp(catalog,connections_catalog) || (count && !destinations))return 0;
  Routing next={0};unsigned seen=0;
  for(int i=0;i<count;i++){
    unsigned j=destinations[i];
    if(j>=8 || (seen&(1u<<j)) || connection_aps[i].inside==connection_aps[j].inside)return 0;
    if(destinations[j]!=i)return 0;
    seen|=1u<<j;next.targets[i]=j;
  }
  next.enabled=count!=0;configurations[slot]=next;return 1;
}
void sm_connections_reset(void){memset(configurations,0,sizeof(configurations));memset(&applied,0,sizeof(applied));selected=-1;cancel_spark=0;}
void sm_connections_activate(int slot){selected=slot>=0 && slot<4?slot:-1;}
void sm_connections_capture(uint8_t *rom){
  captured=rom;memset(&applied,0,sizeof(applied));cancel_spark=0;
  for(int i=0;i<8;i++){
    memcpy(original_doors[i],rom+0x10000+connection_aps[i].door,12);
    original_cre[i]=rom[0x70008+connection_aps[i].room];
    for(int n=0;n<connection_aps[i].song_count;n++)memcpy(original_songs[i]+n*2,rom+0x70000+connection_aps[i].songs[n],2);
  }
}
void sm_connections_restore(uint8_t *rom){
  if(rom!=captured)return;
  for(int i=0;i<8;i++){
    memcpy(rom+0x10000+connection_aps[i].door,original_doors[i],12);
    rom[0x70008+connection_aps[i].room]=original_cre[i];
    for(int n=0;n<connection_aps[i].song_count;n++)memcpy(rom+0x70000+connection_aps[i].songs[n],original_songs[i]+n*2,2);
  }
}
void sm_connections_apply(uint8_t *rom,int randomized){
  if(rom!=captured)return;
  applied=randomized && selected>=0?configurations[selected]:(Routing){0};cancel_spark=0;
  if(!applied.enabled)return;
  for(int i=0;i<8;i++){
    unsigned j=applied.targets[i];const Connection *c=&connection_matrix[i][j];
    uint16_t room=connection_aps[j].room;
    const uint8_t bytes[]={room,room>>8,c->flag,c->direction,c->cap_x,c->cap_y,c->screen_x,c->screen_y,c->distance,c->distance>>8,c->asm_ptr,c->asm_ptr>>8};
    memcpy(rom+0x10000+connection_aps[i].door,bytes,12);
    if(connection_aps[i].door==0x91ce)rom[0x70008+room]=2;
    for(int n=0;n<connection_aps[j].song_count;n++){
      uint8_t *p=rom+0x70000+connection_aps[j].songs[n];p[0]=connection_aps[j].song;p[1]=5;
    }
  }
}
int sm_connections_door(void){
  if(!sm_seed_active() || !applied.enabled)return 0;
  int index=-1;for(int i=0;i<8;i++)if(connection_aps[i].door==door_def_ptr){index=i;break;}
  if(index<0)return 0;
  const Connection *c=&connection_matrix[index][applied.targets[index]];
  sm_connections_arrival(c->asm_ptr,c->incompatible,c->x,c->y,c->exit_fix);
  return 1;
}
void sm_connections_arrival(uint16_t asm_ptr,int incompatible,uint16_t x,uint16_t y,int exit_fix){
  if(incompatible){
    samus_x_pos=x;samus_y_pos=y;
    samus_y_subspeed=samus_y_speed=samus_x_extra_run_speed=samus_x_extra_run_subspeed=samus_x_base_speed=samus_x_base_subspeed=0;
    memset(g_ram+0xa1c,0,12);memset(g_ram+0xa2a,0xff,6);samus_anim_frame=0;
    if(samus_contact_damage_index==2)cancel_spark=1;
    samus_contact_damage_index=0;memcpy(g_ram+0xb10,g_ram+0xaf6,8);
  }
  samus_invincibility_timer=128;
  if(asm_ptr)CallDoorDefSetupCode(0x8f0000|asm_ptr);
  if(exit_fix==1){
    /* VARIA enters the first helper at $A7C77D, after its unpause-hook store. */
    uint16_t previous=unpause_hook.addr;
    Kraid_FadeInBg_LoadBg3Tiles1of4();unpause_hook.addr=previous;
    Kraid_FadeInBg_LoadBg3Tiles2of4();Kraid_FadeInBg_LoadBg3Tiles3of4();Kraid_FadeInBg_LoadBg3Tiles4of4();
  }else if(exit_fix==2)nmi_flag_bg2_enemy_vram_transfer=0;
}
void sm_connections_room(void){
  if(!sm_seed_active() || (!applied.enabled && !sm_minimizer_active()) || room_ptr!=boss_save_plm.room || roomdefroomstate_ptr!=boss_save_plm.state)return;
  const uint8_t *entry=boss_save_plm.entry;
  unsigned header=entry[0]|entry[1]<<8,block=2*(entry[3]*room_width_in_blocks+entry[2]);
  for(int i=0;i<40;i++)if(plm_header_ptr[i]==header && plm_block_indices[i]==block)return;
  uint8_t previous[6];memcpy(previous,g_ram+0x12,6);
  memcpy(g_ram+0x12,entry,6);SpawnRoomPLM(0x12);memcpy(g_ram+0x12,previous,6);
}
uint16_t sm_connections_spark_health(void){
  if(cancel_spark){cancel_spark=0;return 16;}
  return samus_health;
}
int sm_connections_test_destination(int index,int *room,int *door,int *x,int *y){
  if(!applied.enabled || index<0 || index>=8)return 0;
  int j=applied.targets[index];const Connection *c=&connection_matrix[index][j];
  *room=connection_aps[j].room;*door=connection_aps[index].door;*x=c->x;*y=c->y;return 1;
}
