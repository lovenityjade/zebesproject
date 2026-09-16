#include "sm_relic.h"
#include "sm_seed.h"
#include "sm_generation.h"
#include "sm_effects.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
typedef struct {char seed[65];uint8_t minutes,seconds,centiseconds,valid;} Checkpoint;
typedef struct {char magic[8];Checkpoint slot[3];uint32_t checksum;} Disk;
static Disk disk;
static char path[4128];
static uint8_t durations[3];
static int active,loaded,room=-1,departed,warning_pending,acknowledged;
static uint32_t checksum(const void *data,size_t size){const uint8_t *p=data;uint32_t h=2166136261u;while(size--)h=(h^*p++)*16777619u;return h;}
int sm_relic_escape_configure(int slot,int minutes){
  if(slot<0||slot>2 || (minutes!=3&&minutes!=5&&minutes!=6&&minutes!=7&&minutes!=10))return 0;
  durations[slot]=minutes;return 1;
}
void sm_relic_escape_session(void){active=0;loaded=1;room=-1;departed=warning_pending=acknowledged=0;}
void sm_relic_escape_open(const char *save){
  memset(&disk,0,sizeof(disk));memcpy(disk.magic,"ZBRELIC1",8);memset(durations,5,3);
  snprintf(path,sizeof(path),"%s.relic-escape",save);
  Disk candidate;FILE *f=fopen(path,"rb");if(f){size_t n=fread(&candidate,1,sizeof(candidate),f);fclose(f);if(n==sizeof(candidate)&&!memcmp(candidate.magic,disk.magic,8)&&candidate.checksum==checksum(&candidate,offsetof(Disk,checksum)))disk=candidate;}
  sm_relic_escape_session();
}
static void persist(void){
  disk.checksum=checksum(&disk,offsetof(Disk,checksum));char tmp[4140];snprintf(tmp,sizeof(tmp),"%s.tmp",path);
  FILE *f=fopen(tmp,"wb");if(!f)return;int ok=fwrite(&disk,1,sizeof(disk),f)==sizeof(disk);if(fclose(f))ok=0;if(ok)rename(tmp,path);
}
void sm_relic_escape_checkpoint(int slot){
  if(slot<0||slot>2||!path[0])return;
  Checkpoint *c=&disk.slot[slot];memset(c,0,sizeof(*c));
  if(active&&!departed){snprintf(c->seed,sizeof(c->seed),"%s",sm_seed_fingerprint());c->valid=2;c->minutes=timer_minutes;c->seconds=timer_seconds;c->centiseconds=timer_centiseconds;}
  persist();
}
void sm_relic_escape_slot_action(int action,int slot,int other){
  if(slot<0||slot>2)return;
  if(action==2&&other>=0&&other<3)disk.slot[other]=disk.slot[slot];
  else if(action==0||action==3||action==4)memset(&disk.slot[slot],0,sizeof(Checkpoint));
  else return;
  persist();
}
int sm_relic_escape_active(void){return active&&!departed && sm_relic_required()>0;}
void sm_relic_escape_acknowledge(void){if(warning_pending){acknowledged=1;warning_pending=0;}}
uint16_t sm_relic_escape_damage(uint16_t damage){
  if(!sm_relic_escape_active())return damage;
  return damage>0x3fff?0x7fff:damage*2;
}
int32_t sm_relic_escape_periodic_damage(int32_t damage){
  if(!sm_relic_escape_active() || damage<=0)return damage;
  return damage>INT32_MAX/2?INT32_MAX:damage*2;
}
void sm_relic_escape_stop(void){departed=1;timer_status=0;earthquake_timer=0;
  if(frame_handler_gamma==FUNC16(Samus_Func3))frame_handler_gamma=FUNC16(nullsub_152);}
static void timer_gfx(void){
  if(vram_write_queue_tail>0x1f0)return;
  VramWriteEntry *v=gVramWriteEntry(vram_write_queue_tail);
  v->size=0x400;v->src.addr=0xc000;v->src.bank=0xb0;v->vram_dst=0x7e00;vram_write_queue_tail+=7;
}
void sm_relic_escape_frame(void){
  if(game_state!=8 || sm_message_active() || queued_message_box_index || !sm_relic_required() || departed)return;
  if(!active && sm_relic_count()>=sm_relic_required()){
    int slot=sm_slots_current();if(slot<0||slot>2)return;
    Checkpoint *c=&disk.slot[slot];
    int restore=loaded&&(c->valid==1||c->valid==2)&&c->seed[64]==0&&c->minutes<=0x10&&c->seconds<=0x59&&c->centiseconds<=0x99&&((c->minutes&15)<=9)&&((c->seconds&15)<=9)&&((c->centiseconds&15)<=9)&&!strcmp(c->seed,sm_seed_fingerprint())&&CheckEventHappened(14);
    /* The stress gift is equipment, not a collected location: the tracker still
     * knows which original Space Jump check has actually been visited. */
    collected_items|=0x200;equipped_items|=0x200;
    if(!acknowledged && !(restore&&c->valid==2)){
      if(!warning_pending){warning_pending=1;sm_relic_escape_warning();}
      return;
    }
    ClearTimerRam();int n=durations[slot]?durations[slot]:5;SetTimerMinutes(((n/10)*16+n%10)<<8);
    if(restore){timer_minutes=c->minutes;timer_seconds=c->seconds;timer_centiseconds=c->centiseconds;}
    timer_status=0x8003;frame_handler_gamma=FUNC16(Samus_Func3);SetEventHappened(14);active=1;loaded=0;room=-1;
    memset(music_queue_track,0,16);memset(music_queue_delay,0,16);
    music_queue_read_pos=music_queue_write_pos;music_timer=music_entry=0;
    room_music_data_index=room_music_track_index=0;
    QueueMusic_Delayed8(0);QueueMusic_Delayed8(0xff24);QueueMusic_Delayed8(7);
  }
  if(!active)return;
  if(frame_handler_gamma==FUNC16(nullsub_152))frame_handler_gamma=FUNC16(Samus_Func3);
  if(room!=room_ptr){room=room_ptr;timer_gfx();room_music_data_index=room_music_track_index=0;}
  if(!(nmi_frame_counter_word%48)){
    unsigned phase=nmi_frame_counter_word/48;
    sm_visual_sprite(samus_x_pos+(int)(phase*71%192)-96,samus_y_pos+(int)(phase*37%128)-64,143);
    QueueSfx2_Max6(0x24);
  }
}
