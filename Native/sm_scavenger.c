#include "sm_scavenger.h"
#include "sm_seed.h"
#include "sm_objective_events.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
#include "sm_scavenger.inc"
#include "sm_scavenger_door.inc"
_Static_assert(sizeof(SmScavengerPlan)==44,"Scavenger ABI changed");
static SmScavengerPlan configurations[4],applied;
static int selected=-1,browse=-1,prompt,action;
static uint8_t *captured,original_door[SCAVENGER_DOOR_BYTES];
/* Original VARIA word, included in the original checksummed save block. */
#define scav_index UNUSED_word_7ED8F0[2]
static int active(void){return sm_seed_active() && applied.count;}
static int progress(void){return scav_index<=applied.count?scav_index:-1;}
static void reset_browse(void){browse=-1;prompt=action=0;}
const char *sm_scavenger_catalog_sha256(void){return scavenger_catalog;}
void sm_scavenger_reset(void){memset(configurations,0,sizeof(configurations));memset(&applied,0,sizeof(applied));selected=-1;reset_browse();}
int sm_scavenger_configure(int slot,const SmScavengerPlan *p,const char *catalog){
  if(slot<0 || slot>=4 || !catalog || strcmp(catalog,scavenger_catalog))return 0;
  if(!p){memset(&configurations[slot],0,sizeof(*p));return 1;}
  if(p->version!=1 || p->size!=sizeof(*p) || !p->count || p->count>17)return 0;
  unsigned seen=0;
  for(int i=0;i<17;i++){
    if(i>=p->count){if(p->order[i])return 0;continue;}
    unsigned hud=p->order[i]&255;
    if(hud>=17 || p->order[i]!=scavenger_entries[hud] || (seen&(1u<<hud)))return 0;
    seen|=1u<<hud;
  }
  configurations[slot]=*p;return 1;
}
int sm_scavenger_configured(int slot){return slot>=0 && slot<4?configurations[slot].count:0;}
void sm_scavenger_activate(int slot){selected=slot>=0 && slot<4?slot:-1;}
void sm_scavenger_apply(int randomized){applied=randomized && selected>=0?configurations[selected]:(SmScavengerPlan){0};reset_browse();}
void sm_scavenger_new_game(void){if(active())scav_index=0;reset_browse();}
void sm_scavenger_capture(uint8_t *rom){
  captured=rom;unsigned offset=0;
  for(unsigned i=0;i<sizeof(scavenger_door)/sizeof(scavenger_door[0]);i++){
    memcpy(original_door+offset,rom+scavenger_door[i].address,scavenger_door[i].size);offset+=scavenger_door[i].size;
  }
}
void sm_scavenger_restore(uint8_t *rom){
  if(rom!=captured)return;
  unsigned offset=0;
  for(unsigned i=0;i<sizeof(scavenger_door)/sizeof(scavenger_door[0]);i++){
    memcpy(rom+scavenger_door[i].address,original_door+offset,scavenger_door[i].size);offset+=scavenger_door[i].size;
  }
}
void sm_scavenger_apply_rom(uint8_t *rom){
  if(rom!=captured)return;
  int ridley=0;for(int i=0;i<applied.count;i++)ridley|=applied.order[i]>>8==0xaa;
  if(!ridley)return;
  for(unsigned i=0;i<sizeof(scavenger_door)/sizeof(scavenger_door[0]);i++)
    memcpy(rom+scavenger_door[i].address,scavenger_door[i].data,scavenger_door[i].size);
}
int sm_scavenger_allows(unsigned location){
  if(!active())return 1;
  int pos=progress();
  /* A corrupt index must not turn a future objective into a free pickup. */
  for(int i=pos<0?0:pos+1;i<applied.count;i++)if(applied.order[i]>>8==location)return 0;
  return 1;
}
int sm_scavenger_pickup(unsigned location){
  if(!sm_scavenger_allows(location))return 0;
  if(active()){
    int pos=progress();
    if(pos>=0 && pos<applied.count && applied.order[pos]>>8==location){
      sm_objective_event_mark(147);scav_index=pos+1;
      if(scav_index==applied.count)sm_objective_event_mark(129);
      reset_browse();
    }
  }
  return 1;
}
int sm_scavenger_ridley_appears(void){return area_index!=2 || sm_scavenger_allows(0xaa);}
void sm_scavenger_ridley_dead(void){
  if(!active())return;
  sm_scavenger_pickup(0xaa);
  SetEventHappened(80); /* Source Ridley flag (same bit as the original boss). */
}
void sm_scavenger_frame(int hud_enabled){
  if(!active() || !hud_enabled || game_state!=15 || progress()<0 || progress()==applied.count){reset_browse();return;}
  if(browse<0){browse=progress();prompt=1;action=0;return;}
  if(action){
    if(prompt){prompt=0;browse=progress();}
    else {browse+=action;if(browse<progress())browse=progress();if(browse>=applied.count)browse=applied.count-1;}
    action=0;return;
  }
  if(joypad1_newkeys&0x40)action=1;
  else if(joypad1_newkeys&0x4000)action=-1;
}
int sm_scavenger_state(int field){
  if(!active())return field==2?-1:0;
  int pos=progress();
  switch(field){case 0:return applied.count;case 1:return pos;
    case 2:return pos>=0 && pos<applied.count?applied.order[pos]>>8:-1;
    case 3:return pos==applied.count && sm_objective_event(129);
    case 4:return prompt;case 5:return pos<0 || pos==applied.count?0:(browse>=0?browse:pos)+1;
    case 6:return sm_objective_event(147);default:return 0;}
}
int sm_scavenger_entry(int index){return active() && index>=0 && index<applied.count?applied.order[index]:-1;}
const char *sm_scavenger_label(void){
  if(!active())return 0;
  if(prompt)return "Press X-Y ";
  int pos=progress();if(pos<0)return "Invalid hunt";
  if(pos==applied.count)return scavenger_labels[17];
  return scavenger_labels[applied.order[browse>=0?browse:pos]&255];
}
