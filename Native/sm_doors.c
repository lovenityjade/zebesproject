#include "sm_doors.h"
#include "sm_seed.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
#include "sm_doors.inc"
_Static_assert(DOOR_COLOR_COUNT==SM_DOOR_COLOR_COUNT,"Door color catalog size changed");
typedef struct {uint8_t colors[DOOR_COLOR_COUNT],enabled;} Doors;
static Doors configurations[4],applied;
static int selected=-1;
static uint8_t original_data[DOOR_DATA_SIZE],original_locations[DOOR_COLOR_COUNT][6],*captured;
const char *sm_doors_catalog_sha256(void){return door_catalog;}
int sm_doors_configure(int slot,const uint8_t *colors,int count,const char *catalog){
  if(slot<0 || slot>=4 || (count!=0 && count!=DOOR_COLOR_COUNT) || (count && !colors) || !catalog || strcmp(catalog,door_catalog))return 0;
  Doors next={0};
  for(int i=0;i<count;i++){
    if(colors[i]>=9 || (!door_locations[i].can_random && colors[i]))return 0;
    next.colors[i]=colors[i];
  }
  next.enabled=count!=0;configurations[slot]=next;return 1;
}
int sm_doors_configured(int slot){return slot>=0 && slot<4 && configurations[slot].enabled;}
int sm_doors_map_color(int index){
  if(!sm_seed_active() || !captured || index<0 || index>=DOOR_COLOR_COUNT)return 0;
  if(applied.enabled)return applied.colors[index];
  const uint8_t *p=captured+door_locations[index].address;unsigned plm=p[0]|p[1]<<8;
  for(int color=1;color<9;color++)if(color_plms[color][door_locations[index].facing]==plm)return color;
  return 0;
}
void sm_doors_reset(void){memset(configurations,0,sizeof(configurations));memset(&applied,0,sizeof(applied));selected=-1;}
void sm_doors_activate(int slot){selected=slot>=0 && slot<4?slot:-1;}
void sm_doors_capture(uint8_t *rom){
  captured=rom;memset(&applied,0,sizeof(applied));
  for(unsigned i=0;i<sizeof(door_spans)/sizeof(*door_spans);i++)memcpy(original_data+door_spans[i].offset,rom+door_spans[i].address,door_spans[i].size);
  for(int i=0;i<DOOR_COLOR_COUNT;i++)memcpy(original_locations[i],rom+door_locations[i].address,6);
}
void sm_doors_restore(uint8_t *rom){
  if(rom!=captured)return;
  for(unsigned i=0;i<sizeof(door_spans)/sizeof(*door_spans);i++)memcpy(rom+door_spans[i].address,original_data+door_spans[i].offset,door_spans[i].size);
  for(int i=0;i<DOOR_COLOR_COUNT;i++){
    /* Restore only fields written by Door.writeColor. Other room bytes may
     * belong to separately restored starts/layout/indicator patches. */
    uint8_t *p=rom+door_locations[i].address;p[0]=original_locations[i][0];p[1]=original_locations[i][1];p[5]=original_locations[i][5];
  }
  memset(&applied,0,sizeof(applied));
}
void sm_doors_apply(uint8_t *rom,int randomized){
  if(rom!=captured)return;
  applied=randomized && selected>=0?configurations[selected]:(Doors){0};
  if(!applied.enabled)return;
  for(unsigned i=0;i<sizeof(door_spans)/sizeof(*door_spans);i++)memcpy(rom+door_spans[i].address,door_data+door_spans[i].offset,door_spans[i].size);
  for(int i=0;i<DOOR_COLOR_COUNT;i++){
    unsigned color=applied.colors[i];if(!color)continue;
    uint8_t *p=rom+door_locations[i].address;uint16_t plm=color_plms[color][door_locations[i].facing];p[0]=plm;p[1]=plm>>8;
    if(color==4)p[5]=0x90; /* Sealed even during escape, exactly as VARIA. */
  }
}
void sm_doors_initialize_save(void){
  if(!applied.enabled)return;
  for(int i=0;i<DOOR_COLOR_COUNT;i++)if(!applied.colors[i]){
    unsigned bit=door_locations[i].opened_bit;opened_door_bit_array[bit>>3]|=1<<(bit&7);
  }
}
int sm_doors_plm_direction(unsigned plm,int indicators_only){
  for(unsigned i=0;i<sizeof(beam_plms)/sizeof(*beam_plms);i++)if(beam_plms[i].plm==plm && (!indicators_only || beam_plms[i].indicator))return beam_plms[i].direction;
  return -1;
}
int sm_doors_preinstr(unsigned address,unsigned k){
  unsigned mask;
  switch(address){
    case 0x84f2db:mask=1;break;
    case 0x84f2ea:mask=2;break;
    case 0x84f2f9:mask=4;break;
    case 0x84f31b:mask=8;break;
    case 0x848560:mask=0;break;
    default:return 0;
  }
  unsigned i=k>>1,shot=plm_timers[i];int open=0;
  if(mask){
    if((shot&0xf00)!=0x500){
      open=(shot&mask)!=0;
      if((mask==4 || mask==8) && (collected_beams&12)==12)open=(shot&12)!=0;
    }
  }else if(shot){
    open=(shot&0xf00)==0x100;
    if(!open)QueueSfx2_Max6(0x57);
  }
  plm_timers[i]=0;
  if(open){plm_instr_list_ptrs[i]=plm_instruction_list_link_reg[i];plm_instruction_timer[i]=1;}
  return 1;
}
