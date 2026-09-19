#include "sm_rush_runtime.h"
#include "sm_runs.h"
#include "sm_seed.h"
#include "sm_generation.h"
#include "sm_map_browser.h"
#include "sm_profile.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
typedef struct {
  uint8_t category,ngplus,started,finished,eligible,completed,gear_pending,reserved;
  uint16_t gear[15],completion[15];
  uint64_t frames,real_ms;
  uint32_t invalid_reason;
  char id[37];
} Run;
typedef struct {char magic[8];Run slots[3];uint32_t checksum;} Disk;
static uint8_t halves[32];static uint16_t previous_enemy[32],previous_room;
static Disk disk;static char path[4128],save_path[4096];
static uint64_t tick,real_remainder;static int current=-1,dirty;
static uint32_t hash(const void *data,size_t n){const uint8_t *p=data;uint32_t h=2166136261u;while(n--)h=(h^*p++)*16777619u;return h;}
static int read_disk(const char *save,Disk *out){
  if(!save || strlen(save)>4095)return 0;
  char file[4128];snprintf(file,sizeof(file),"%s.runs",save);FILE *f=fopen(file,"rb");if(!f)return 0;
  size_t n=fread(out,1,sizeof(*out),f);fclose(f);
  return n==sizeof(*out) && !memcmp(out->magic,"ZBRUNS01",8) && out->checksum==hash(out,offsetof(Disk,checksum));
}
void sm_runs_open(const char *save){
  memset(&disk,0,sizeof(disk));memcpy(disk.magic,"ZBRUNS01",8);Disk old;if(read_disk(save,&old))disk=old;
  snprintf(path,sizeof(path),"%s.runs",save);snprintf(save_path,sizeof(save_path),"%s",save);
  memset(halves,0,sizeof(halves));memset(previous_enemy,0,sizeof(previous_enemy));previous_room=0;
  current=-1;dirty=0;real_remainder=0;tick=sm_clock_ns();
}
int sm_runs_save(void){
  if(!dirty||!path[0])return 1;
  disk.checksum=hash(&disk,offsetof(Disk,checksum));char tmp[4140];snprintf(tmp,sizeof(tmp),"%s.tmp",path);
  FILE *f=fopen(tmp,"wb");if(!f)return 0;int ok=fwrite(&disk,1,sizeof(disk),f)==sizeof(disk);if(fclose(f))ok=0;
  if(ok && !rename(tmp,path)){dirty=0;return 1;}return 0;
}
static Run *active(void){int s=sm_slots_current();if(s<0)s=selected_save_slot;return !sm_seed_active()&&s>=0&&s<3?&disk.slots[s]:0;}
int sm_run_state(int field){Run *r=active();if(!r)return 0;switch(field){case 0:return r->category;case 1:return r->ngplus;case 2:return r->started&&r->eligible;case 3:return r->finished;default:return 0;}}
int sm_run_assists_allowed(void){return sm_run_state(0)!=1;}
int sm_run_configure(int slot,int category){
  if(slot<0||slot>2||category<0||category>2||disk.slots[slot].started||(nonempty_save_slots&(1<<slot)))return 0;
  disk.slots[slot].category=category;dirty=1;return sm_runs_save();
}
int sm_run_has_completion(const char *save,int slot){Disk d;return slot>=0&&slot<3&&read_disk(save,&d)&&d.slots[slot].completed&&!d.slots[slot].ngplus;}
int sm_run_import_ngplus(const char *source,int from,int to){
  Disk d;if(from<0||from>2||to<0||to>2||disk.slots[to].started||(nonempty_save_slots&(1<<to))||!read_disk(source,&d)||!d.slots[from].completed||d.slots[from].ngplus)return 0;
  Run *r=&disk.slots[to];memcpy(r->gear,d.slots[from].completion,sizeof(r->gear));r->ngplus=1;dirty=1;return sm_runs_save();
}
static void capture(uint16_t *g){
  uint16_t data[]={equipped_items,collected_items,equipped_beams,collected_beams,samus_health,samus_max_health,samus_missiles,samus_max_missiles,samus_super_missiles,samus_max_super_missiles,samus_power_bombs,samus_max_power_bombs,samus_reserve_health,samus_max_reserve_health,reserve_health_mode};memcpy(g,data,sizeof(data));
}
static void apply(const uint16_t *g){
  equipped_items=g[0];collected_items=g[1];equipped_beams=g[2];collected_beams=g[3];samus_health=g[4];samus_max_health=g[5];samus_missiles=g[6];samus_max_missiles=g[7];samus_super_missiles=g[8];samus_max_super_missiles=g[9];samus_power_bombs=g[10];samus_max_power_bombs=g[11];samus_reserve_health=g[12];samus_max_reserve_health=g[13];reserve_health_mode=g[14];
  Samus_LoadSuitPalette();UpdateBeamTilesAndPalette();
}
void sm_run_new_game(void){
  Run *r=active();if(!r)return;
  /* This hook runs only for a genuinely fresh Start, never a save load.
   * Restarting before the first station still needs a new run and NG+ gear. */
  r->finished=0;r->invalid_reason=0;
  r->started=r->eligible=1;r->gear_pending=r->ngplus;r->frames=r->real_ms=0;real_remainder=0;current=selected_save_slot;tick=sm_clock_ns();
  unsigned char id[16];
#ifdef _WIN32
  int ok=sm_windows_random(id,sizeof(id));
#else
  FILE *f=fopen("/dev/urandom","rb");int ok=f&&fread(id,1,16,f)==16;if(f)fclose(f);
#endif
  if(!ok){uint64_t t=sm_clock_ns();memcpy(id,&t,8);memcpy(id+8,&t,8);}id[6]=(id[6]&15)|64;id[8]=(id[8]&63)|128;
  snprintf(r->id,sizeof(r->id),"%02x%02x%02x%02x-%02x%02x-%02x%02x-%02x%02x-%02x%02x%02x%02x%02x%02x",id[0],id[1],id[2],id[3],id[4],id[5],id[6],id[7],id[8],id[9],id[10],id[11],id[12],id[13],id[14],id[15]);dirty=1;sm_runs_save();
}
void sm_run_invalidate(int reason){if(sm_rush_active())return;Run *r=active();if(r&&r->started&&!r->finished){r->eligible=0;if(!r->invalid_reason)r->invalid_reason=reason;dirty=1;}}
void sm_runs_frame(void){
  uint64_t now=sm_clock_ns(),elapsed=now-tick;tick=now;
  Run *r=active();if(!r)return;
  if(game_state==8){
    if(current!=selected_save_slot){current=selected_save_slot;elapsed=0;}
    if(r->gear_pending){apply(r->gear);r->gear_pending=0;dirty=1;}
    if(!r->started){/* Old saves can unlock NG+ but cannot claim a complete speedrun. */r->started=1;r->eligible=0;dirty=1;}
  }
  if(current==selected_save_slot && r->started&&!r->finished && game_state>=6 && game_state<40){
    r->frames=(((uint64_t)game_time_hours*60+game_time_minutes)*60+game_time_seconds)*60+game_time_frames;
    real_remainder+=elapsed;r->real_ms+=real_remainder/1000000;real_remainder%=1000000;dirty=1;
  }
}
uint64_t sm_run_time(int real){Run *r=active();if(!r)return 0;return real?r->real_ms:r->frames;}
void sm_run_finish(void){if(sm_rush_active())return;
  Run *r=active();if(!r||r->finished)return;
  r->frames=(((uint64_t)game_time_hours*60+game_time_minutes)*60+game_time_seconds)*60+game_time_frames;
  if(!r->ngplus&&!r->completed){capture(r->completion);r->completed=1;}
  r->finished=1;dirty=1;sm_runs_save();
  if(!r->category||!r->id[0])return;
  char file[4200],tmp[4210];snprintf(file,sizeof(file),"%s.run-%s.json",save_path,r->id);snprintf(tmp,sizeof(tmp),"%s.tmp",file);
  FILE *f=fopen(tmp,"wb");if(!f)return;
  int ok=fprintf(f,"{\"schema\":1,\"run_id\":\"%s\",\"ruleset\":\"zebes-v1\",\"mode\":\"%s\",\"category\":\"%s\",\"igt_frames\":%llu,\"real_ms\":%llu,\"eligible\":%s,\"invalid_reason\":%u}\n",r->id,r->ngplus?"ngplus":"vanilla",r->category==1?"noqol":"qol",(unsigned long long)r->frames,(unsigned long long)r->real_ms,r->eligible?"true":"false",r->invalid_reason)>0;
  if(fclose(f))ok=0;if(ok)rename(tmp,file);
}
void sm_run_slot_action(int action,int slot,int other){
  if(slot<0||slot>2)return;
  if(action==2&&other>=0&&other<3){disk.slots[other]=disk.slots[slot];disk.slots[other].eligible=0;disk.slots[other].invalid_reason=4;}
  else if(action==0||action==3||action==4)memset(&disk.slots[slot],0,sizeof(Run));
  else return;dirty=1;sm_runs_save();
}
/* Half incoming damage gives exactly twice the endurance without overflowing
 * SNES health or moving scripted boss phase thresholds. Fractional hits carry. */

void sm_run_enemy_spawn(int index){halves[(index>>6)&31]=0;}
uint32_t sm_run_enemy_damage(uint32_t amount){
  if(sm_rush_active())return sm_rush_enemy_damage(amount);
  if(!sm_run_state(1))return amount;
  int i=(cur_enemy_index>>6)&31;
  if(previous_room!=room_ptr){memset(halves,0,32);memset(previous_enemy,0,sizeof(previous_enemy));previous_room=room_ptr;}
  if(previous_enemy[i]!=enemy_data[i].enemy_ptr){halves[i]=0;previous_enemy[i]=enemy_data[i].enemy_ptr;}
  uint32_t result=(amount+halves[i])/2;halves[i]=(amount+halves[i])&1;return result;
}
uint16_t sm_run_contact_damage(uint16_t amount){return !sm_rush_active()&&sm_run_state(1)?(uint16_t)(((uint32_t)amount*3/2)>65535?65535:(uint32_t)amount*3/2):amount;}
extern uint8_t sm_wide_hud[];
void sm_run_render(uint8_t *pixels){
  if(sm_rush_active())return;
  if(!sm_run_state(0)||game_state<8||game_state>18||game_state==15)return;
  uint64_t f=sm_run_time(0);char text[24];snprintf(text,sizeof(text),"%02u:%02u:%02u",(unsigned)(f/216000),(unsigned)(f/3600%60),(unsigned)(f/60%60));
  uint32_t color=sm_run_state(2)?0xffffff:0xff6868;
  sm_native_map_text(pixels,256,8,33,text,color);sm_native_map_text((uint8_t*)sm_ui_overlay(),256,8,33,text,color);sm_native_map_text(sm_wide_hud,400,8,33,text,color);
}
