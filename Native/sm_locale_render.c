#include "sm_locale.h"
#include "sm_bridge.h"
#include "sm_scene.h"
#include "sm_map_browser.h"
#include "sm_seed_atlas.h"
#include "ida_types.h"
#include "variables.h"
#include "sm_rtl.h"
#include "snes/snes.h"
#include "snes/ppu.h"
#include <string.h>
extern uint8_t sm_wide_hud[];
extern Snes* g_snes;
/* Derive the compact pause alphabet from the player's original ROM at runtime.
 * No Nintendo bitmap is embedded here. Each run is one existing letter, with
 * its original width; accents occupy the two blank rows above the glyph. */
static uint8_t glyph[26][8], widths[26];
static struct {const char* text;int x,y,n,total,color;} captions[4];
static int caption_count;
void sm_locale_caption(const char* text,int x,int y,int visible,int total,int color){
 if(caption_count>=4)return;
 captions[caption_count++]=(typeof(captions[0])){text,x,y,visible,total,color};
}
int sm_locale_area_label(unsigned id,unsigned x,unsigned y,unsigned attributes){
 if(!sm_locale_get() || (game_state!=5 && !sm_map_browser_overview()))return 0;
 unsigned address=0x82c749;unsigned base=GET_WORD(RomPtr(address));
 const char* text=id==base?"PLANÈTE ZEBES":id==base+4?"ÉPAVE":NULL;
 if(!text)return 0;
 sm_locale_caption(text,x,y,1,1,attributes?0x4210:0x7fff);return 1;
}
static void draw_captions(uint8_t* pixels){
 for(int i=0;i<caption_count;i++){
  const char* text=captions[i].text;int length=sm_locale_length(text),n=(length*captions[i].n+captions[i].total-1)/captions[i].total;
  int x=captions[i].x-length*4,y=captions[i].y-4,color=captions[i].color;
  // sm_native_map_text applies the current native fade exactly once.
  int rgb=((color&31)*255/31)<<16 | (((color>>5)&31)*255/31)<<8 | ((color>>10)&31)*255/31;
  char visible[128];int bytes=0;
  while(*text && n-->0){const char* first=text;sm_locale_next(&text);int count=text-first;if(bytes+count>=sizeof(visible))break;memcpy(visible+bytes,first,count);bytes+=count;}visible[bytes]=0;
  if(y<0 || y>214)continue;
  sm_native_map_text(pixels,256,x,y,visible,rgb);
  sm_native_map_text((uint8_t*)sm_ui_overlay(),256,x,y,visible,rgb);
  sm_native_map_text(sm_wide_hud,400,x+72,y,visible,rgb);
 }
 caption_count=0;
}
static void font(void){
 if(widths[0])return;
 static const struct {const char* letters;int n;uint16_t tiles[8];} words[]={
  {"CHARGE",4,{0xd8,0xd9,0xda,0xe7}}, {"ICE",3,{0xdb,0xdc,0xd4}},
  {"WAVE",3,{0xdd,0xde,0xdf}}, {"SPAZER",4,{0xe8,0xe9,0xea,0xeb}},
  {"PLASMA",4,{0xec,0xed,0xee,0xef}}, {"VARIASUIT",6,{0x100,0x101,0x102,0x103,0x104,0x105}},
  {"GRAVITYSUIT",7,{0xd0,0xd1,0xd2,0xd3,0x103,0x104,0x105}},
  {"MORPHINGBALL",8,{0x120,0x121,0x122,0x123,0x117,0x118,0x10f,0x11f}},
  {"SCREWATTACK",7,{0xe0,0xe1,0xe2,0xe3,0xe4,0xe5,0xe6}},
  {"SPACEJUMP",6,{0xf0,0xf1,0xf2,0xf3,0xf4,0xf5}},
  {"SPEEDBOOSTER",8,{0x124,0x125,0x126,0x127,0x128,0x129,0x12a,0x12b}},
 };
 const uint8_t* rom=RomFixedPtr(0xb68000);
 for(unsigned w=0;w<sizeof(words)/sizeof(*words);w++){
  uint8_t columns[64]={0};int count=words[w].n*8;
  for(int x=0;x<count;x++)for(int y=0;y<8;y++){
   const uint8_t* p=rom+words[w].tiles[x/8]*32;int c=0;
   for(int b=0;b<4;b++)c|=((p[y*2+b/2*16+b%2]>>(7-x%8))&1)<<b;
   if(c==10)columns[x]|=1<<y;
  }
  int letter=0;
  for(int x=0;x<count && words[w].letters[letter];){
   if(!columns[x]){x++;continue;}int start=x;while(x<count && columns[x])x++;
   int c=words[w].letters[letter++]-'A';if(widths[c])continue;
   widths[c]=x-start;for(int i=start;i<x && i-start<8;i++)glyph[c][i-start]=columns[i];
  }
 }
 /* Remaining letters use the game's item-message alphabet, trimmed without
  * resampling. This also keeps future text additions legible. */
 for(int c=0;c<26;c++)if(!widths[c]){
  const uint8_t* p=RomFixedPtr(0x9ab200)+(0xe0+c)*16;int first=8,last=-1;
  for(int x=0;x<8;x++)for(int y=0;y<6;y++)if((((p[y*2]>>(7-x))&1)|(((p[y*2+1]>>(7-x))&1)<<1))==1){if(x<first)first=x;if(x>last)last=x;}
  widths[c]=last>=first?last-first+1:4;
  for(int x=first;x<=last;x++)for(int y=0;y<6;y++)if((((p[y*2]>>(7-x))&1)|(((p[y*2+1]>>(7-x))&1)<<1))==1)glyph[c][x-first]|=1<<(y+2);
 }
}
static int advance(int cp){int c=sm_locale_base(cp);return c>='A'&&c<='Z'?widths[c-'A']+1:c==' '?3:3;}
static int length(const char* s){int n=0;while(*s)n+=advance(sm_locale_next(&s));return n?n-1:0;}
static void dot(uint8_t* out,int w,int x,int y,const uint8_t color[4]){if(x>=0&&x<w&&y>=0&&y<224)memcpy(out+(y*w+x)*4,color,4);}
static void compact(uint8_t* out,int w,int x,int y,const char* s,const uint8_t color[4]){
 while(*s){int cp=sm_locale_next(&s),c=sm_locale_base(cp),a=sm_locale_accent(cp);
  if(c>='A'&&c<='Z'){
   for(int dx=0;dx<widths[c-'A'];dx++)for(int dy=0;dy<8;dy++)if(glyph[c-'A'][dx]&(1<<dy))dot(out,w,x+dx,y+dy,color);
   int mid=widths[c-'A']/2;
   if(a==1){dot(out,w,x+mid+1,y,color);dot(out,w,x+mid,y+1,color);}
   if(a==2){dot(out,w,x+mid-1,y,color);dot(out,w,x+mid,y+1,color);}
   if(a==3){dot(out,w,x+mid,y,color);dot(out,w,x+mid-1,y+1,color);dot(out,w,x+mid+1,y+1,color);}
   if(a==4){dot(out,w,x,y+1,color);dot(out,w,x+widths[c-'A']-1,y+1,color);}
   if(a==5)dot(out,w,x+mid,y+7,color);
  }else if(c=='.')dot(out,w,x,y+6,color);
  else if(c=='-')for(int dx=0;dx<2;dx++)dot(out,w,x+dx,y+4,color);
  else if(c=='\'')dot(out,w,x+1,y+1,color);
  x+=advance(cp);
 }
}
static void fill(uint8_t* out,int w,int x,int y,int width,int height,const uint8_t color[4]){for(int dy=0;dy<height;dy++)for(int dx=0;dx<width;dx++)dot(out,w,x+dx,y+dy,color);}
static void panel(uint8_t* out,int w,int x,int y,int width,const char* label,int enabled){
 // Use the native equipment palette and current fade; never infer ownership
 // from translated strings or change the native selection/toggle state.
 const Ppu* p=g_snes->ppu;int bright=p->forcedBlank?0:p->brightness;
 uint8_t colors[2][4];for(int i=0;i<2;i++){int c=p->cgram[(enabled?2:3)*16+11-i];for(int b=0;b<3;b++)colors[i][b]=((c>>(5*(2-b)))&31)*255/31*bright/15;colors[i][3]=255;}
 fill(out,w,x,y,width,8,colors[0]);compact(out,w,x+1,y,label,colors[1]);
}
static void heading(uint8_t* out,int w,int x,int y,int width,const char* label){
 int b=g_snes->ppu->forcedBlank?0:g_snes->ppu->brightness;
 uint8_t bg[]={0,0,0,255},fg[]={b*17,b*17,b*17,255};
 fill(out,w,x,y,width,8,bg);compact(out,w,x+(width-length(label))/2,y,label,fg);
}
static void pause(uint8_t* out,int w){
 int right=w==400?144:0,center=(w-256)/2;
 if(pause_screen_mode==1){
  heading(out,w,36,72,44,"RÉSERVE");heading(out,w,38,120,34,"RAYONS");
  heading(out,w,186+right,64,44,"ARMURE");heading(out,w,180+right,96,56,"ÉQUIPEMENT");heading(out,w,185+right,144,48,"MOBILITÉ");
  if(samus_max_reserve_health){heading(out,w,32,80,56,reserve_health_mode==1?"MODE AUTO":"MODE MANUEL");heading(out,w,32,88,56,"RÉS. AUX.");}
  const char* beams[]={"CHARGE","GLACE","ONDES","SPAZER","PLASMA"};
  const uint16_t* masks=(const uint16_t*)RomFixedPtr(0x82c04c);
  for(int i=0;i<5;i++)if(collected_beams&masks[i])panel(out,w,40,128+i*8,32,beams[i],(equipped_beams&masks[i])!=0);
  const char* suits[]={"VARIA","GRAVITÉ","MORPHOSPHÈRE","BOMBES","BOULE DE SAUT","ATQ. VRILLE"};
  masks=(const uint16_t*)RomFixedPtr(0x82c056);
  for(int i=0;i<6;i++)if(collected_items&masks[i])panel(out,w,176+right,i<2?72+i*8:104+(i-2)*8,64,suits[i],(equipped_items&masks[i])!=0);
  const char* boots[]={"BOTTES SAUT","SAUT SPATIAL","ACCÉLÉRATEUR"};masks=(const uint16_t*)RomFixedPtr(0x82c062);
  for(int i=0;i<3;i++)if(collected_items&masks[i])panel(out,w,176+right,152+i*8,64,boots[i],(equipped_items&masks[i])!=0);
  // Only the caption inside the original L button changes.
  uint8_t bg[4],fg[]={0,0,0,255};memcpy(bg,out+(204*w+44)*4,4);
  // Solid button color is at its top-left interior, away from the old glyph.
  memcpy(bg,out+(201*w+42)*4,4);fill(out,w,44,201,28,13,bg);compact(out,w,45,204,"CARTE",fg);
 }
 if(pause_screen_mode==2){
  uint8_t bg[4],fg[]={0,0,0,255};memcpy(bg,out+(201*w+w-74)*4,4);
  fill(out,w,w-72,201,28,13,bg);compact(out,w,w-71,204,"CARTE",fg);
 }
 if(!pause_screen_mode && !sm_seed_atlas_active() && sm_map_browser_view_area()==3){
  uint8_t bg[]={0,0,0,255};fill(out,w,w/2-56,32,112,16,bg);
  sm_native_map_text(out,w,w/2-20,40,"ÉPAVE",0xffffff);
 }
 // Preserve controller names (L/R/START); translate the action on the button.
 uint8_t red[]={0,0,(g_snes->ppu->forcedBlank?0:g_snes->ppu->brightness)*17,255},black[]={0,0,0,255};
 fill(out,w,100+center,202,24,9,red);compact(out,w,105+center,203,"JEU",black);
}
static void energy(uint8_t* out,int w){
 int b=g_snes->ppu->forcedBlank?0:g_snes->ppu->brightness;
 uint8_t black[]={0,0,0,255},white[]={b*17,b*17,b*17,255};
 fill(out,w,8,23,39,9,black);compact(out,w,8,23,"ÉNERGIE",white);
}
void sm_locale_render(uint8_t* pixels){
 if(!sm_locale_get()){caption_count=0;return;}font();sm_locale_colony_frame();draw_captions(pixels);
 if((game_state>=8 && game_state<=18) && !sm_map_browser_overview()){energy(pixels,256);energy((uint8_t*)sm_ui_overlay(),256);energy(sm_wide_hud,400);}
 if(game_state>=14 && game_state<=16 && !sm_map_browser_overview()){
  pause(pixels,256);pause((uint8_t*)sm_ui_overlay(),256);pause(sm_wide_hud,400);
 }
}
