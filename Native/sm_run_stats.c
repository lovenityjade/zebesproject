#include "sm_run_stats.h"
#include "sm_runs.h"
#include "sm_route.h"
#include "sm_varia_ui.h"
#include "sm_bridge.h"
#include "ida_types.h"
#include "variables.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
/* Sidecar keeps the original SRAM format and native slot checksums intact.
 * started: 0 = unknown history, 1 = tracked run, 2 = generated but never started. */
typedef struct {char seed[65];uint8_t started,partial,finished;uint64_t value[48];} Record;
typedef struct {char magic[8];Record slot[3];uint64_t checksum;} Disk;
static Disk data;
static char path[4128];
static int current=-1,previous,menu_seen,loaded_slot,dirty;
static uint64_t hash(const void *p,size_t n){const uint8_t *b=p;uint64_t h=14695981039346656037ull;while(n--){h^=*b++;h*=1099511628211ull;}return h;}
void sm_stats_open(const char *save){
  memset(&data,0,sizeof(data));memcpy(data.magic,"ZBSTATS1",8);snprintf(path,sizeof(path),"%s.stats",save);
  Disk loaded;FILE *f=fopen(path,"rb");if(f){size_t n=fread(&loaded,1,sizeof(loaded),f);fclose(f);if(n==sizeof(loaded)&&!memcmp(loaded.magic,data.magic,8)&&loaded.checksum==hash(&loaded,offsetof(Disk,checksum)))data=loaded;}
  current=-1;previous=0;menu_seen=1;loaded_slot=0;dirty=0;
}
int sm_stats_save(void){
  if(!dirty || !path[0])return 1;
  data.checksum=hash(&data,offsetof(Disk,checksum));char tmp[4140];snprintf(tmp,sizeof(tmp),"%s.tmp",path);
  FILE *f=fopen(tmp,"wb");if(!f)return 0;int ok=fwrite(&data,1,sizeof(data),f)==sizeof(data);if(fclose(f))ok=0;
  if(ok && rename(tmp,path)==0){dirty=0;return 1;}return 0;
}
uint64_t sm_stats_value(int id){return current>=0&&id>=0&&id<48?data.slot[current].value[id]:0;}
int sm_stats_partial(void){return current<0 || data.slot[current].partial;}
void sm_stats_add(int id){if(current>=0 && id>=0&&id<48 && !data.slot[current].finished){data.slot[current].value[id]++;dirty=1;}}
void sm_stats_finish(void){sm_run_finish();sm_route_finish();if(current>=0){data.slot[current].finished=1;dirty=1;sm_stats_save();}}
void sm_stats_slot_action(int action,int slot,int other){
  if(slot<0||slot>2)return;
  if(action==2 && other>=0&&other<3)data.slot[other]=data.slot[slot];
  else if(action==0||action==3||action==4){
    Record *r=&data.slot[slot];memset(r,0,sizeof(*r));
    if(action==4){snprintf(r->seed,sizeof(r->seed),"%s",sm_seed_fingerprint());r->started=2;}
  }
  else return;
  dirty=1;sm_stats_save();
}
void sm_stats_frame(void){
  int s=game_state;
  if(s==2||s==4){menu_seen=1;loaded_slot=selected_save_slot<3 && (nonempty_save_slots&(1<<selected_save_slot));}
  if(s==8 && (current<0 || menu_seen || current!=selected_save_slot)){
    current=selected_save_slot<3?selected_save_slot:0;
    Record *r=&data.slot[current];const char *seed=sm_seed_active()?sm_seed_fingerprint():"vanilla";
    if(strcmp(r->seed,seed)){memset(r,0,sizeof(*r));snprintf(r->seed,sizeof(r->seed),"%s",seed);}
    if(r->started==1){r->value[41]++;r->finished=0;}
    else {r->partial=r->started!=2 && (loaded_slot||game_time_hours||game_time_minutes||game_time_seconds>5);r->started=1;}
    menu_seen=0;dirty=1;
  }
  if(current>=0 && !menu_seen && !data.slot[current].finished){
    Record *r=&data.slot[current];r->value[0]++;dirty=1;
    r->value[1]=(((uint64_t)game_time_hours*60+game_time_minutes)*60+game_time_seconds)*60+game_time_frames;
    if(s>=13&&s<=16)r->value[38]++;
    else if((s>=6&&s<=18) || s==27){int region=sm_varia_region();if(region>=0&&region<12)r->value[7+region*2]++;}
    if((s==9||s==10||s==11)&&previous==8)r->value[2]++;
    if(s>=9&&s<=11)r->value[3]++;
    if(s==8 && nmi_frames_missed && !sm_message_active())r->value[42]++;
    if((s==19 && previous!=19)||(s==36 && previous!=36)){r->value[40]++;sm_stats_save();}
    if(s==39)sm_stats_finish();
  }
  previous=s;
}
