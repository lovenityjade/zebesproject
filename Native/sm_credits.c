#include "sm_soundtrack.h"
#include "sm_relic.h"
#include "sm_credits.h"
#include "sm_run_stats.h"
#include "sm_bridge.h"
#include "sm_scene.h"
#include "sm_rtl.h"
#include "sm_cpu_infra.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include "spc_player.h"
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "sm_credits_assets.inc"
/* Original artwork is decoded only from the verified player's ROM. */
void sm_credits_load_assets(void){
  uint8_t tiles[65536],map_bytes[65536],original_bytes[65536];
  DecompressToMem(0x97e7de,credits_font);
  memcpy(credits_palette,RomPtr(0x8ce9e9),sizeof(credits_palette));
  memcpy(credits_planet_palette,RomPtr(0x8ce5e9),sizeof(credits_planet_palette));
  DecompressToMem(0x96ec76,tiles);DecompressToMem(0x978adb,map_bytes);
  DecompressToMem(0x97eeff,original_bytes);
  for(int y=0;y<224;y++)for(int x=0;x<256;x++){
    unsigned at=(y/8*32+x/8)*2;uint16_t id=map_bytes[at]|map_bytes[at+1]<<8;
    int px=id&0x4000?7-x%8:x%8,py=id&0x8000?7-y%8:y%8;
    unsigned off=(id&1023)*32+py*2,ci=0;
    for(int b=0;b<4;b++)ci|=((tiles[off+b/2*16+b%2]>>(7-px))&1)<<b;
    credits_planet[y*256+x]=ci?((id>>10)&7)*16+ci:0;
  }
  for(unsigned r=0;r<sizeof(credits_source_rows)/sizeof(*credits_source_rows);r++){
    int src=credits_source_rows[r];
    if(src<0){for(int x=0;x<32;x++)credits_upstream[r][x]=0x7f;}
    else if(src<128)memcpy(credits_upstream[r],original_bytes+src*64,64);
    else for(unsigned j=0;j<sizeof(credits_varia_row_ids)/sizeof(*credits_varia_row_ids);j++)
      if(credits_varia_row_ids[j]==src)memcpy(credits_upstream[r],credits_varia_rows[j],64);
  }
}

uint16 CinematicFunction_Intro_Func219(uint16 k,uint16 j);
extern uint8_t sm_wide_pixels[],sm_wide_hud[];
static uint16_t rows[1024][32];
static int count,mode,frame,ending_frame,buttons_before,section_start[5];
static SpcPlayer *preview_music;
static uint8_t saved[256*240*4*3+400*240*4*2];
static void copy_buffers(int restore){
  uint8_t *ptrs[]={(uint8_t*)sm_pixels(),sm_scene_pixels,(uint8_t*)sm_ui_overlay(),sm_wide_pixels,sm_wide_hud};
  size_t sizes[]={256*240*4,256*240*4,256*240*4,400*240*4,400*240*4};size_t offset=0;
  for(int i=0;i<5;i++){if(restore)memcpy(ptrs[i],saved+offset,sizes[i]);else memcpy(saved+offset,ptrs[i],sizes[i]);offset+=sizes[i];}
}
static void blank(int n){while(n-- && count<1024){for(int i=0;i<32;i++)rows[count][i]=0x7f;count++;}}
static void small(const char *s,int palette){
  blank(1);int len=strlen(s);if(len>32)len=32;int x=(32-len)/2;
  for(int i=0;i<len;i++)rows[count-1][x+i]=credits_small_font[(unsigned char)s[i]&127]|palette<<10;
}
static uint16_t big_glyph(char ch,int bottom){
  if(ch>='a'&&ch<='z')ch=toupper((unsigned char)ch);
  uint16_t t=credits_big_font[(unsigned char)ch&127];
  if(bottom){if(ch>='A'&&ch<='Z')t=credits_big_font[(unsigned char)tolower(ch)];else if((ch>='0'&&ch<='9')||ch=='%')t+=16;else if(ch=='\'')t=0x7f;else if(ch==':')t=0x5a;else if(ch=='.')t=0x5a;}
  else if(ch=='.')t=0x7f;else if(ch==':')t=0x5a;
  return t;
}
static void big(const char *s){
  blank(2);int len=strlen(s);if(len>32)len=32;int x=(32-len)/2;
  for(int i=0;i<len;i++){rows[count-2][x+i]=big_glyph(s[i],0);rows[count-1][x+i]=big_glyph(s[i],1);}
}
static void stat(const char *label,int id,int time){
  char value[32],line[33];uint64_t v=sm_stats_value(id);
  if(time)snprintf(value,sizeof(value),"%02llu:%02llu:%02llu.%02llu",(unsigned long long)(v/216000),(unsigned long long)(v/3600%60),(unsigned long long)(v/60%60),(unsigned long long)(v%60));
  else snprintf(value,sizeof(value),"%llu",(unsigned long long)v);
  memset(line,' ',32);line[32]=0;size_t n=strlen(label),m=strlen(value);if(n>17)n=17;if(m>13)m=13;
  memcpy(line+1,label,n);memcpy(line+31-m,value,m);big(line);blank(1);
}
static void build(void){
  count=0;blank(25);section_start[0]=count;
  small("THE ZEBES PROJECT",4);blank(2);big("THELOVENITYJADE");blank(2);big("SEKAILINK");big("SEKAILINK.COM");blank(6);
  section_start[1]=count;small("NATIVE DECOMPILATION",5);blank(2);big("SNESREV");blank(1);big("DABANANA64   LYWX");blank(2);small("NATIVE RUNTIME",6);big("ELZO_D");blank(2);
  small("DISASSEMBLY REFERENCES",6);blank(1);big("STRAGER - MATTHEW GLAZAR");big("BLAKE SMITH");big("ANONYMOUS CONTRIBUTORS");blank(2);big("PJBOY   KEJARDON");blank(4);
  section_start[2]=count;
  for(unsigned i=0;i<sizeof(credits_upstream)/sizeof(credits_upstream[0]);i++){memcpy(rows[count++],credits_upstream[i],64);}
  blank(3);small("VARIA.RUN",6);small("DISCORD.VARIA.RUN",6);blank(5);
  small("REMASTERED SOUNDTRACK",5);blank(2);
  small("MUSIC RESTORATION",6);big("JAMMIN' SAM MILLER");blank(2);
  small("ORCHESTRAL ARRANGEMENTS",6);
  big("BLAKE ROBINSON");small("THE SYNTHETIC ORCHESTRA",6);blank(1);
  big("THE NOBLE DEMON");big("GMB SOUND TEAM");big("PONTUS HULTGREN MUSIC");
  big("VG MUSIC REVISITED");big("DJ ATOMNIUM");big("WINGUS DINGUS");blank(2);
  small("INTRO VOICE",6);big("LUKE CORREIA");blank(2);
  small("MSU PACK COMPILATION",6);big("JUD6MENT");big("NONAMESD");blank(2);
  small("MSU MUSIC INTEGRATION",6);big("DARKSHOCK");big("CUBEAR");blank(5);
  small("ENGINE AND MIDDLEWARE",5);blank(2);
  small("THE ZEBES PROJECT USES",6);small("UNREAL ENGINE.",6);
  small("UNREAL IS A TRADEMARK OR",6);small("REGISTERED TRADEMARK OF",6);
  small("EPIC GAMES, INC. IN THE",6);small("UNITED STATES OF AMERICA",6);small("AND ELSEWHERE.",6);blank(2);
  small("UNREAL ENGINE",6);small("COPYRIGHT 1998 - 2026",6);
  small("EPIC GAMES, INC.",6);small("ALL RIGHTS RESERVED.",6);blank(2);
  small("DEAR IMGUI - OMAR CORNUT",6);small("AND CONTRIBUTORS",6);blank(4);
  section_start[3]=count;small("GAMEPLAY STATISTICS",5);blank(2);
  if(sm_stats_partial()){small("TRACKED SINCE THIS UPDATE",6);blank(2);}
  small(sm_seed_active()?"RANDOMIZED GAME":"VANILLA GAME",4);blank(2);
  if(sm_relic_required()){char relics[33];small("CHOZO RELIC HUNT",4);snprintf(relics,sizeof(relics),"FRAGMENTS %d OF %d",sm_relic_count(),sm_relic_required());big(relics);blank(2);}
  stat("REAL TIME",0,1);stat("IN GAME TIME",1,1);stat("DEATHS",40,0);stat("RESETS",41,0);
  stat("DOOR TRANSITIONS",2,0);stat("TIME IN DOORS",3,1);stat("DOOR ALIGNMENT",5,1);stat("PAUSE MENU",38,1);
  small("TIME SPENT IN",2);blank(2);
  static const char *regions[]={"CERES","CRATERIA","GREEN BRINSTAR","RED BRINSTAR","WRECKED SHIP","KRAID'S LAIR","UPPER NORFAIR","CROCOMIRE","LOWER NORFAIR","WEST MARIDIA","EAST MARIDIA","TOURIAN"};
  for(int i=0;i<12;i++)stat(regions[i],7+i*2,1);
  small("SHOTS AND AMMO FIRED",3);blank(2);
  static const char *shots[]={"UNCHARGED SHOTS","CHARGED SHOTS","BEAM ATTACKS","MISSILES","SUPER MISSILES","POWER BOMBS","BOMBS"};
  for(int i=0;i<7;i++)stat(shots[i],31+i,0);
  stat("LAG TIME",42,1);blank(5);section_start[4]=count;big("THANKS FOR PLAYING");blank(32);
  ending_frame=count*16;
}
void sm_credits_reset(void){sm_credits_close();mode=frame=0;}
int sm_credits_state(int field){if(field==0)return mode;if(field==1)return frame;if(field==2)return ending_frame;if(field==3)return count;if(field==4){int n=0;for(int i=0;i<5;i++)if(frame/16+14>=section_start[i])n=i;return n;}return 0;}
int sm_credits_launch(void){
  if(!g_snes || mode || (game_state!=8&&game_state!=15) || sm_message_active())return 0;
  copy_buffers(0);build();frame=0;mode=2;buttons_before=0;
  sm_soundtrack_preview_begin();
  preview_music=SpcPlayer_Create();SpcPlayer_Initialize(preview_music);SpcPlayer_Upload(preview_music,RomPtr(0xcf8000));
  const uint8_t *p=RomPtr(0x8fe7e1+0x3c);uint32_t a=p[0]|p[1]<<8|p[2]<<16;SpcPlayer_Upload(preview_music,RomPtr(a));preview_music->input_ports[0]=5;
  return 1;
}
void sm_credits_close(void){
  sm_soundtrack_preview_end();
  if(mode==2)copy_buffers(1);
  if(preview_music){free(preview_music->dsp);free(preview_music);preview_music=0;}
  mode=0;
}
void sm_credits_start_ending(void){sm_stats_finish();build();frame=0;mode=1;}
int sm_credits_native_tick(void){
  if(mode!=1)return 0;
  CinematicUpdateSomeBg(); /* Keep native ending tilemap DMA alive beneath the custom roll. */
  if(++frame>=ending_frame){mode=0;cinematic_var21=0;CinematicFunction_Intro_Func219(0,0);}
  return 1;
}
static void pixel(uint8_t *p,int w,int x,int y,uint16_t c,float light){
  if(x<0||x>=w||y<0||y>=224)return;uint8_t *d=p+(y*w+x)*4;
  d[0]=fminf(255,((c>>10)&31)*255/31*light);d[1]=fminf(255,((c>>5)&31)*255/31*light);d[2]=fminf(255,(c&31)*255/31*light);d[3]=255;
}
static void glyph(uint8_t *p,int w,int x,int y,uint16_t id){
  int tile=id&1023,pal=(id>>10)&7;if(tile>=sizeof(credits_font)/32)return;
  for(int dy=0;dy<8;dy++)for(int dx=0;dx<8;dx++){
    int off=tile*32+((id&32768)?7-dy:dy)*2,bit=(id&16384)?dx:7-dx,c=0;
    for(int b=0;b<4;b++)c|=((credits_font[off+b/2*16+b%2]>>bit)&1)<<b;
    if(c)pixel(p,w,x+dx,y+dy,credits_palette[pal*16+c],1);
  }
}
static void draw(uint8_t *scene,uint8_t *gui,int w){
  memset(scene,0,w*240*4);memset(gui,0,w*240*4);
  for(int i=3;i<w*240*4;i+=4)scene[i]=255;
  int cx=w/2+52,cy=104;
  // Actual Zebes approach artwork, translated intact; stars remain original pixels.
  for(int y=0;y<224;y++)for(int x=0;x<256;x++){
    int ci=credits_planet[y*256+x];if(!ci)continue;
    int planet=x>=72&&x<176&&y>=48&&y<168;
    float twinkle=planet?.62f:.30f+.45f*(.5f+.5f*sinf(frame*.035f+x*.73f+y*.21f));
    pixel(scene,w,x+cx-123,y+cy-104,credits_planet_palette[ci],twinkle);
  }
  // Sparse extra stars only extend the authored sky into the widescreen wings.
  for(int i=0;i<70;i++){unsigned h=i*2654435761u+12345;int x=h%(unsigned)w,y=(h>>16)%224;if(x>cx-123&&x<cx+133)continue;
    pixel(scene,w,x,y,0x7fff,.12f+.35f*powf(.5f+.5f*sinf(frame*.02f+i),4));}
  int scroll=frame/2;
  for(int row=scroll/8;row<count && row*8-scroll<224;row++)for(int x=0;x<32;x++)glyph(gui,w,(w-256)/2+x*8,row*8-scroll,rows[row][x]);
}
void sm_credits_render(uint8_t *pixels){
  if(!mode)return;
  draw(sm_scene_pixels,(uint8_t*)sm_ui_overlay(),256);draw(sm_wide_pixels,sm_wide_hud,400);
  const uint8_t *ui=sm_ui_overlay();for(int i=0;i<256*240;i++)memcpy(pixels+i*4,(ui[i*4+3]?ui:sm_scene_pixels)+i*4,4);
}
int sm_credits_preview_step(uint16_t buttons,uint8_t *pixels,int16_t *audio){
  if(mode!=2)return 0;
  if((buttons&8)&&!(buttons_before&8) || frame>=ending_frame){sm_credits_close();memset(audio,0,736*2*2);return 1;}
  buttons_before=buttons;frame+=(buttons&128)?8:1;
  SpcPlayer_GenerateSamples(preview_music);dsp_getSamples(preview_music->dsp,audio,736);
  sm_soundtrack_preview_mix(audio,736);
  sm_credits_render(pixels);return 1;
}
