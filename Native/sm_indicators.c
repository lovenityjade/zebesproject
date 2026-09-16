#include "sm_indicators.h"
#include "sm_seed.h"
#include "sm_doors.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
#include "sm_indicators.inc"
_Static_assert(sizeof(indicator_locations)/sizeof(*indicator_locations)==SM_INDICATOR_MAX,"Indicator location catalog size changed");
static SmDoorIndicator selected[SM_INDICATOR_MAX],applied[SM_INDICATOR_MAX];
static int selected_count,applied_count;
static uint8_t original[SM_INDICATOR_MAX][6],*captured;
static int direction(unsigned plm){
  if(plm>=0xfbb0 && plm<=0xfc0a && (plm-0xfbb0)%6==0)return ((plm-0xfbb0)/6)%4;
  return sm_doors_plm_direction(plm,1);
}
int sm_indicators_is_plm(unsigned plm){return direction(plm)>=0;}
int sm_indicators_validate(const SmDoorIndicator *entries,int count,uint64_t patches){
  if(count<0 || count>SM_INDICATOR_MAX || (count && (!entries || !(patches&INDICATOR_PATCH_MASK))))return 0;
  uint32_t seen=0;
  for(int i=0;i<count;i++){
    unsigned id=entries[i].location,plm=entries[i].plm;
    if(id>=SM_INDICATOR_MAX || (seen&(1u<<id)) || !sm_indicators_is_plm(plm))return 0;
    if(direction(plm)!=indicator_locations[id].direction)return 0;
    seen|=1u<<id;
  }
  return 1;
}
void sm_indicators_select(const SmDoorIndicator *entries,int count){
  if(count)memcpy(selected,entries,count*sizeof(*entries));selected_count=count;
}
void sm_indicators_capture(uint8_t *rom){
  captured=rom;selected_count=applied_count=0;
  for(int i=0;i<SM_INDICATOR_MAX;i++)if(indicator_locations[i].address)
    memcpy(original[i],rom+indicator_locations[i].address,6);
}
void sm_indicators_restore(uint8_t *rom){
  if(rom!=captured)return;
  for(int i=0;i<SM_INDICATOR_MAX;i++)if(indicator_locations[i].address)
    memcpy(rom+indicator_locations[i].address,original[i],6);
}
void sm_indicators_apply(uint8_t *rom,int enabled){
  if(rom!=captured)return;
  applied_count=enabled?selected_count:0;
  if(applied_count)memcpy(applied,selected,applied_count*sizeof(*selected));
  for(int i=0;i<applied_count;i++){
    unsigned id=applied[i].location;uint32_t address=indicator_locations[id].address;
    if(address){
      uint8_t *p=rom+address;p[0]=applied[i].plm;p[1]=applied[i].plm>>8;
      p[2]=indicator_locations[id].x;p[3]=indicator_locations[id].y;
      p[4]=indicator_locations[id].argument;p[5]=indicator_locations[id].argument>>8;
    }
  }
}
void sm_indicators_room(void){
  if(!sm_seed_active())return;
  for(int i=0;i<applied_count;i++){
    unsigned id=applied[i].location;
    if(indicator_locations[id].room!=room_ptr)continue;
    unsigned block=2*(indicator_locations[id].y*room_width_in_blocks+indicator_locations[id].x);
    int found=0;
    for(int j=0;j<40;j++)if(plm_header_ptr[j]==applied[i].plm && plm_block_indices[j]==block)found=1;
    if(found)continue;
    uint8_t previous[6];memcpy(previous,g_ram+0x12,6);
    uint8_t entry[]={applied[i].plm,applied[i].plm>>8,indicator_locations[id].x,indicator_locations[id].y,indicator_locations[id].argument,indicator_locations[id].argument>>8};
    memcpy(g_ram+0x12,entry,6);SpawnRoomPLM(0x12);memcpy(g_ram+0x12,previous,6);
  }
}
