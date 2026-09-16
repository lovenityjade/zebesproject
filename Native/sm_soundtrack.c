/* External MSU-1 playback for the native port. Cue mapping follows DarkShock's
 * SuperMetroid-MSU1 mapping, as retained in VARIA's supermetroid_msu1.asm.
 * PCM: MSU1 + u32le loop frame + 44100 Hz signed 16-bit stereo frames.
 * All I/O/mixing occurs on the game thread, with bounded block reads. */
#include "sm_soundtrack.h"
#include <stdio.h>
#include <string.h>
#include <limits.h>

typedef struct {
  FILE *file;
  uint64_t frames, position, loop;
  int track, looping, done;
} Music;
static Music main_music, preview_music, gameover_music;
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
void sm_soundtrack_gameover_configure(const char *filename){
  const char *path=filename?filename:"";
  if(strlen(path)>=sizeof(gameover_path))return;
  if(strcmp(gameover_path,path)){stop(&gameover_music);strcpy(gameover_path,path);}
}
int sm_soundtrack_gameover_status(void){return gameover_music.file && !gameover_music.done;}
uint8_t sm_soundtrack_command(unsigned bank,uint8_t command) {
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
static int mix(Music *m,int16_t *audio,int frames) {
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
      int value=audio[i]+sample;
      audio[i]=value>INT16_MAX?INT16_MAX:value<INT16_MIN?INT16_MIN:value;
    }
    audio+=n*2;frames-=n;m->position+=n;
  }
  return 1;
}
void sm_soundtrack_mix(int16_t *audio,int frames) {
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
void sm_soundtrack_reset(void) { stop(&gameover_music);stop(&main_music);sm_soundtrack_preview_end();fallback=have_command=0;last_bank=last_command=0;error[0]=0; }
int sm_soundtrack_status(int field) {
  const Music *m=preview?&preview_music:&main_music;
  switch(field){case 0:return enabled;case 1:return m->track;case 2:return m->file&&!m->done;case 3:return fallback;case 4:return preview;default:return 0;}
}
uint64_t sm_soundtrack_position(void) { return (preview?&preview_music:&main_music)->position; }
const char *sm_soundtrack_error(void) { return error; }
