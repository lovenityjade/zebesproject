#include "sm_finale.h"
#include "sm_ending.h"
/* External MSU-1 playback for the native port. Cue mapping follows DarkShock's
 * SuperMetroid-MSU1 mapping, as retained in VARIA's supermetroid_msu1.asm.
 * PCM: MSU1 + u32le loop frame + 44100 Hz signed 16-bit stereo frames.
 * All I/O/mixing occurs on the game thread, with bounded block reads. */
#include "sm_soundtrack.h"
#include "sm_rush_runtime.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>
#include <math.h>
#include "sm_rush_death_audio.inc"

typedef struct {
  FILE *file;
  uint64_t frames, position, loop;
  int track, looping, done;
} Music;
static Music main_music, preview_music, gameover_music;
static Music rush_music, rush_transition;
static char rush_paths[5][4096];
static int rush_attempt, rush_valid, rush_cue=-1;
static float rush_gain=32768.f;
static int finale_owner;
static uint64_t finale_audio_frames;
static char gameover_path[4096];
static char root[4096], error[256];
static int enabled, preview, fallback, have_command;
static unsigned last_bank;
static uint8_t last_command;
static uint32_t le32(const unsigned char *p) { return p[0]|(uint32_t)p[1]<<8|(uint32_t)p[2]<<16|(uint32_t)p[3]<<24; }
static void stop(Music *m) { if(m->file)fclose(m->file);memset(m,0,sizeof(*m)); }
static int cue(unsigned bank, uint8_t cmd) {
  static const uint8_t map[25][3]={
    {4,5},{4,5},{6,0,7},{8,9},{10},{11},{12},{13},{14},{15,16},
    {17,0},{18},{19,21,20},{22,23},{24},{0,25,0},{26,27},{0},
    {28},{29},{30},{100},{101},{22,23},{10}};
  if(cmd>0 && cmd<4)return cmd;
  if(cmd<5 || cmd>7 || bank%3 || bank/3>=25)return 0;
  return map[bank/3][cmd-5];
}
static int open_music(Music *m,const char *path,int track,int looping) {
  stop(m);
  unsigned char header[8];
  FILE *f=fopen(path,"rb");
  if(!f){snprintf(error,sizeof(error),"Track %d is unavailable; using Original.",track);return 0;}
  if(fseek(f,0,SEEK_END))goto invalid;
  long size=ftell(f);
  if(size<12 || (size-8)%4 || fseek(f,0,SEEK_SET) || fread(header,1,8,f)!=8 || memcmp(header,"MSU1",4))goto invalid;
  m->file=f;m->track=track;m->frames=(size-8)/4;m->loop=le32(header+4);
  if(m->loop>=m->frames)m->loop=0; /* Same out-of-range fallback as bsnes. */
  m->looping=looping;
  error[0]=0;return 1;
invalid:
  fclose(f);snprintf(error,sizeof(error),"Track %d has invalid PCM data; using Original.",track);return 0;
}
static int open_track(Music *m,int track) {
  char path[4200];
  if(!root[0] || snprintf(path,sizeof(path),"%s/zebes-%d.pcm",root,track)>=(int)sizeof(path)){stop(m);return 0;}
  return open_music(m,path,track,track!=1 && track!=2 && track!=29 && track!=30 && track!=100 && track!=101);
}
int sm_soundtrack_rush_configure(const char *theme,const char *enter,const char *exit) {
  const char *paths[3]={theme,enter,exit};
  char previous_error[sizeof(error)];memcpy(previous_error,error,sizeof(error));
  rush_impact_reset();stop(&rush_music);stop(&rush_transition);rush_attempt=rush_valid=0;rush_cue=-1;memset(rush_paths,0,sizeof(rush_paths));
  for(int i=0;i<3;i++){
    rush_paths[i][0]=0;
    if(!paths[i] || !paths[i][0] || strlen(paths[i])>=sizeof(rush_paths[i]))continue;
    Music probe={0};
    if(open_music(&probe,paths[i],103+i,i==0)){
      strcpy(rush_paths[i],paths[i]);rush_valid|=1<<i;
    }
    stop(&probe);
  }
  /* Optional Rush files must not overwrite the regular soundtrack status. */
  memcpy(error,previous_error,sizeof(error));return rush_valid;
}
int sm_soundtrack_rush_results_configure(const char *failed,const char *success){
 const char *paths[]={failed,success};char previous_error[sizeof(error)];memcpy(previous_error,error,sizeof(error));
 for(int n=0;n<2;n++){
  int i=n+3;rush_valid&=~(1<<i);rush_paths[i][0]=0;
  if(!paths[n] || !paths[n][0] || strlen(paths[n])>=sizeof(rush_paths[i]))continue;
  Music probe={0};
  if(open_music(&probe,paths[n],103+i,0)){strcpy(rush_paths[i],paths[n]);rush_valid|=1<<i;}
  stop(&probe);
 }
 memcpy(error,previous_error,sizeof(error));return rush_valid;
}
static int rush_sync(void) {
  int finale=sm_finale_music();
  if(finale==2 && !(rush_valid&1)){
    /* Its bank was uploaded during the native transfer, but its play command
     * was suppressed by the sacrifice silence. Resume it if the file is absent. */
    if(finale_owner)sm_soundtrack_spc_command(5);
    finale=0;
  }
  if(finale && !finale_owner){stop(&main_music);sm_soundtrack_spc_command(0);}
  if(finale_owner!=finale){rush_attempt=0;finale_owner=finale;}
  if(!sm_rush_active() && !finale){
    rush_impact_reset();stop(&rush_music);stop(&rush_transition);rush_attempt=0;rush_cue=-1;rush_gain=32768.f;return 0;
  }
  const int stage=sm_rush_info(0);
  int next=stage==SM_RUSH_DEATH?5:stage==SM_RUSH_GAMEOVER?3:stage==SM_RUSH_RESULTS?4:0;
  if(finale)next=finale==1?5:6;
  if(next<5 && !(rush_valid&(1<<next)))next=0; /* Optional result-file fallback. */
  if(!rush_attempt || next!=rush_cue){
    rush_impact_reset();rush_attempt=1;rush_cue=next;rush_gain=32768.f;
    stop(&rush_music);stop(&rush_transition);
    if(next==6){
      finale_audio_frames=0;
      open_music(&rush_music,rush_paths[0],109,1); // Finale cue starts at 0:00.
      rush_gain=0;
    }else if(next<5 && (rush_valid&(1<<next)))open_music(&rush_music,rush_paths[next],103+next,next==0);
  }
  /* A completed one-shot keeps ownership of the music bus. Repeated status
   * polls must neither reopen it nor bring the looping combat theme back. */
  return finale==1 || rush_music.file!=NULL || (rush_cue==5 && (rush_valid&25));
}
void sm_soundtrack_rush_transition(int reverse) {
  if(!rush_sync())return;
  if(reverse && rush_cue==5)rush_impact_start();
  int index=reverse?2:1;
  if(rush_valid&(1<<index))open_music(&rush_transition,rush_paths[index],103+index,0);
}
uint64_t sm_soundtrack_rush_status(int field) {
  if(!rush_sync())return 0;
  switch(field){case 0:return 1;case 1:return rush_music.position;
    case 2:return rush_transition.file && !rush_transition.done;
    case 3:return rush_cue;case 4:return rush_music.done;case 5:return rush_music.frames;default:return 0;}
}
void sm_soundtrack_gameover_configure(const char *filename){
  const char *path=filename?filename:"";
  if(strlen(path)>=sizeof(gameover_path))return;
  if(strcmp(gameover_path,path)){stop(&gameover_music);strcpy(gameover_path,path);}
}
int sm_soundtrack_gameover_status(void){return gameover_music.file && !gameover_music.done;}
uint8_t sm_soundtrack_command(unsigned bank,uint8_t command) {
  /* Only music port APUI00 is intercepted. SPC uploads and all SFX ports keep
   * their native path, including Mother Brain's scripted sound effects. */
  if(rush_sync())return 0;
  if(sm_ending_preview_active())return command; /* Pause the real external track. */
  /* The original Game Over bank is still uploaded: its Metroid cries and
   * cursor SFX remain native. Only its ambience command is replaced. */
  if(sm_state()==26 && bank==3 && command==4 && gameover_path[0]){
    if(gameover_music.file || open_music(&gameover_music,gameover_path,102,1))return 0;
  }
  /* Command 4 layers SPC ambience over external music in the original patch. */
  if(command==4 && main_music.track)return command;
  last_bank=bank;last_command=command;have_command=1;
  if(!enabled){stop(&main_music);fallback=0;return command;}
  if(!command){stop(&main_music);fallback=0;return command;}
  int track=cue(bank,command);
  if(!track){stop(&main_music);fallback=0;return command;}
  if(main_music.track==track)return 0; /* Do not restart duplicate requests. */
  if(open_track(&main_music,track)){fallback=0;return 0;}
  fallback=1;return command;
}
void sm_soundtrack_configure(int remastered,const char *directory) {
  const char *path=directory?directory:"";remastered=!!remastered;
  if(strlen(path)>=sizeof(root)){snprintf(error,sizeof(error),"Soundtrack directory path is too long.");return;}
  if(enabled==remastered && !strcmp(root,path))return;
  enabled=remastered;memcpy(root,path,strlen(path)+1);stop(&main_music);fallback=0;error[0]=0;
  if(have_command)sm_soundtrack_spc_command(sm_soundtrack_command(last_bank,last_command));
  stop(&preview_music);
  if(preview && enabled)open_track(&preview_music,30);
}
static int mix_gain(Music *m,int16_t *audio,int frames,int gain) {
  if(!m->file || m->done)return 1;
  unsigned char block[4096];
  while(frames>0){
    if(m->position==m->frames){
      if(!m->looping){m->done=1;return 1;}
      m->position=m->loop;
      if(fseek(m->file,(long)(8+m->loop*4),SEEK_SET))return 0;
    }
    int n=frames>1024?1024:frames;
    if((uint64_t)n>m->frames-m->position)n=(int)(m->frames-m->position);
    if(fread(block,4,n,m->file)!=(size_t)n)return 0;
    for(int i=0;i<n*2;i++){
      int sample=(int16_t)(block[i*2]|block[i*2+1]<<8);
      int applied=gain;
      if(m==&rush_music){
        if(!(i&1))rush_gain+=(gain-rush_gain)/(gain<rush_gain?256.f:2048.f);
        applied=(int)rush_gain;
        if(rush_cue==6){
          /* Finale only: 1:14 cue, two-second entry. Normal Rush remains cue 0. */
          float t=fminf(1.f,(finale_audio_frames+i/2)/88200.f);
          applied=(int)(applied*t*t*(3.f-2.f*t));
        }
        if(rush_cue==3 || rush_cue==4){
          /* One second from silence to full gain; cursor resets only on entry. */
          float t=fminf(1.f,(m->position+i/2)/44100.f);
          applied=(int)(applied*t*t*(3.f-2.f*t));
        }
      }
      int value=audio[i]+sample*applied/32768;
      audio[i]=value>INT16_MAX?INT16_MAX:value<INT16_MIN?INT16_MIN:value;
    }
    if(m==&rush_music && rush_cue==6)finale_audio_frames+=n;
    audio+=n*2;frames-=n;m->position+=n;
  }
  return 1;
}
static int mix(Music *m,int16_t *audio,int frames) {return mix_gain(m,audio,frames,32768);}
void sm_soundtrack_rush_render(int16_t *audio,int frames) {
  if(!audio || frames<=0 || frames>44100)return;
  memset(audio,0,(size_t)frames*4);
  if(!rush_sync())return;
  /* Leave room for the sweep without changing the volume of native effects. */
  int transition=rush_transition.file && !rush_transition.done;
  if(rush_cue!=5 && !mix_gain(&rush_music,audio,frames,finale_owner==2?sm_finale_gain():(transition?16384:32768))){
    stop(&rush_music);stop(&rush_transition);
    if(have_command)sm_soundtrack_spc_command(last_command);
    return;
  }
  if(!mix_gain(&rush_transition,audio,frames,rush_cue==5?16384:32768))stop(&rush_transition);
  rush_impact_mix(audio,frames);
}
void sm_soundtrack_mix(int16_t *audio,int frames) {
  if(rush_sync())return; /* Unreal streams Rush audio even while Step is frozen. */
  if(sm_state()!=26)stop(&gameover_music);
  if(!mix(&gameover_music,audio,frames)){stop(&gameover_music);sm_soundtrack_spc_command(4);}
  if(!mix(&main_music,audio,frames)){
    stop(&main_music);fallback=1;snprintf(error,sizeof(error),"Audio read failed; using Original.");
    if(have_command)sm_soundtrack_spc_command(last_command);
  }
}
void sm_soundtrack_preview_begin(void) { preview=1;stop(&preview_music);if(enabled)open_track(&preview_music,30); }
void sm_soundtrack_preview_end(void) { stop(&preview_music);preview=0; }
void sm_soundtrack_preview_mix(int16_t *audio,int frames) {
  if(!preview_music.file)return;
  /* The preview's SPC instance keeps advancing silently, ready for a live toggle. */
  int16_t original[736*2];
  if(frames>736)return;
  memcpy(original,audio,frames*4);memset(audio,0,frames*4);
  if(!mix(&preview_music,audio,frames)){stop(&preview_music);memcpy(audio,original,frames*4);}
}
void sm_soundtrack_reset(void) { finale_owner=0; rush_impact_reset();stop(&rush_music);stop(&rush_transition);rush_attempt=0;rush_cue=-1;stop(&gameover_music);stop(&main_music);sm_soundtrack_preview_end();fallback=have_command=0;last_bank=last_command=0;error[0]=0; }
int sm_soundtrack_status(int field) {
  const Music *m=preview?&preview_music:&main_music;
  switch(field){case 0:return enabled;case 1:return m->track;case 2:return m->file&&!m->done;case 3:return fallback;case 4:return preview;default:return 0;}
}
uint64_t sm_soundtrack_position(void) { return (preview?&preview_music:&main_music)->position; }
const char *sm_soundtrack_error(void) { return error; }
