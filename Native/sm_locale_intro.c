#include "sm_locale.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include <string.h>
#include "sm_cpu_infra.h"
#include "sm_rtl.h"
/* Translate glyphs while preserving native script timing, voices and waits. */
static const unsigned bounds[]={0xc383,0xc797,0xcb45,0xce33,0xd15d,0xd511,0xd67d};
static const unsigned counts[]={172,155,123,133,156,31};
static const char* pages[]={
 "J'ai affronté les Métroïdes pour la première fois sur Zebes. J'y ai déjoué le plan de Mother Brain, chef des Pirates de l'espace, qui voulait utiliser ces créatures contre la civilisation galactique...",
 "J'ai ensuite combattu les Métroïdes sur leur planète d'origine, SR388. Je les ai tous éliminés, sauf une larve qui, après son éclosion, m'a suivie comme un enfant perdu...",
 "Je l'ai remise en personne aux scientifiques de la station de recherche galactique Ceres pour qu'ils étudient sa capacité à produire de l'énergie...",
 "Leurs découvertes étaient stupéfiantes! Les pouvoirs du Métroïde pourraient être utilisés pour le bien de la civilisation!",
 "Rassurée, j'ai quitté la station pour chercher une nouvelle prime. Mais je venais à peine de franchir la ceinture d'astéroïdes quand j'ai capté un signal de détresse!",
 "La station Ceres était attaquée!"
};
static int active=-1,seen,drawn,length;
static unsigned short glyphs[320],positions[320],diacritics[320];
static unsigned tile_for(int ch){
 if(ch>='A'&&ch<='Z')return 0xd685+(ch-'A')*6;
 if(ch>='0'&&ch<='9')return 0xd685+(26+ch-'0')*6;
 if(ch=='.')return 0xd75d;if(ch==',')return 0xd763;
 if(ch=='?')return 0xd769;if(ch=='\'')return 0xd76f;
 if(ch=='!')return 0xd77b;return 0xd67d;
}
static void prepare(int page){
 int chars[320],n=0,x=1,y=4;const char* text=pages[page];
 while(*text && n<320)chars[n++]=sm_locale_next(&text);
 length=seen=drawn=0;active=page;
 for(int i=0;i<n;i++){
  if(i==0 || chars[i-1]==' '){int word=0;while(i+word<n && chars[i+word]!=' ')word++;
   if(x+word>30){x=1;y+=2;}}
  if(y>=24 || length>=320)break;
  if(chars[i]==' ' && x==1)continue;
  positions[length]=(y<<8)|x;diacritics[length]=sm_locale_accent(chars[i]);glyphs[length++]=tile_for(sm_locale_base(chars[i]));x++;
 }
}
int sm_locale_intro_char(unsigned k,unsigned j,unsigned position){
 if(!sm_locale_get() || j==0xd683)return 0;
 unsigned pointer=cinematicbg_instr_ptr[k>>1];int page=-1;
 for(int i=0;i<6;i++)if(pointer>=bounds[i] && pointer<bounds[i+1])page=i;
 if(page<0 || (j!=0xd67d && (j<0xd685 || j>0xd77b || (j-0xd685)%6)))return 0;
 if(page!=active || position==0x0401)prepare(page);
 ++seen;int target=(seen*length+counts[page]-1)/counts[page];if(target>length)target=length;
 for(;drawn<target;drawn++){
  unsigned glyph=glyphs[drawn],pos=positions[drawn];
  ProcessCinematicBgObject_DrawToTextTilemap(0x1e,glyph,pos);
  SpawnTextGlowObject(glyph,pos);
  if(diacritics[drawn])ram3000.pause_menu_map_tilemap[((pos>>8)-1)*32+(pos&255)]=0x2090+diacritics[drawn]-1;
  cinematicbg_arr7[15]=8*((pos&255)+1);
  cinematicbg_arr8[15]=8*(pos>>8)-8;
 }
 cinematicbg_var1=!cinematicbg_var1;
 if(j!=0xd67d && cinematicbg_var1)QueueSfx3_Max6(0x0d);
 return 1;
}

void sm_locale_intro_setup(void){
 if(!sm_locale_get())return;
 // The native intro font ends at word 4480; the next tilemap begins at 4800.
 // Accent tiles occupy this otherwise unused gap for this cinematic only.
 static const uint8_t masks[5][2]={{8,16},{32,16},{16,40},{0,36},{0,16}};
 for(int i=0;i<5;i++){
  uint16_t* tile=g_snes->ppu->vram+0x4480+i*8;memset(tile,0,16);
  tile[6]=masks[i][0];tile[7]=masks[i][1];
 }
 static const int cps[]={0xc0,0xc2,0xc7,0xc8,0xc9,0xca,0xcb,0xce,0xcf,0xd4,0xd9,0xdb,0xdc};
 for(int i=0;i<13;i++){
  int c=sm_locale_base(cps[i])-'A',src=0x30+(c/16)*32+c%16;
  uint16_t* tile=g_snes->ppu->vram+0x4500+i*8;
  memcpy(tile,g_snes->ppu->vram+0x4000+src*8,16);
  int a=sm_locale_accent(cps[i])-1;tile[0]=masks[a][0];tile[1]=masks[a][1];
 }
 uint16_t* map=g_snes->ppu->vram+0x4c00;
 for(int i=0;i<1024;i++)map[i]=0x002f;
 const char* lines[]={"LE DERNIER MÉTROÏDE","EST EN CAPTIVITÉ.","LA GALAXIE EST EN PAIX..."};
 for(int l=0;l<3;l++){
  const char* p=lines[l];int x=(32-sm_locale_length(p))/2,y=8+l*3;
  while(*p){int cp=sm_locale_next(&p),c=sm_locale_base(cp)-'A';
   if(c>=0&&c<26){int top=0x30+(c/16)*32+c%16;map[(y+1)*32+x]=0x1000|top+16;
    for(int i=0;i<13;i++)if(cps[i]==cp)top=0xa0+i;
    map[y*32+x]=0x1000|top;
   }else if(cp=='.')map[(y+1)*32+x]=0x1026;
   x++;
  }
 }
}

/* Ending captions are typewritten by a separate native script. Replace only
 * its glyph writes: the percentage, waits, music and ending selection stay
 * native. The alphabet is the original credits font already in BG1. */
static int colony_chars;
void sm_locale_colony_frame(void){
 if(game_state!=30 || (cinematic_function!=0xa38f && cinematic_function!=0xbfda && cinematic_function!=0xc0c5)){colony_chars=0;return;}
 if(colony_chars)sm_locale_caption("COLONIE SPATIALE",124,196,colony_chars,12,0x7fff);
}
int sm_locale_ending_char(unsigned glyph,unsigned position){
 if(!sm_locale_get())return 0;
 // Ceres uses BG glyphs, unlike the later sprite-based Zebes caption.
 if(game_state==30 && glyph>=0xd7c1 && glyph<=0xd7f1 && (glyph-0xd7c1)%6==0){
  colony_chars=(position&255)-9;return 1;
 }
 if(game_state!=39)return 0;
 int row=position>>8,col=position&255,start=0,total=0,big=0;
 const char* text=NULL;
 if(glyph>=0xe189 && glyph<=0xe1e3 && (glyph-0xe189)%6==0){
  if(row==10){text="TAUX D'OBJETS";start=9;total=13;}
  if(row==12){text="RECUEILLIS";start=6;total=19;}
 }else if(glyph>=0xe131 && glyph<=0xe181 && (glyph-0xe131)%8==0 && row==2){
  text="À LA PROCHAINE MISSION";start=6;total=20;big=1;
 }
 if(!text)return 0;
 int length=sm_locale_length(text),target=((col-start+1)*length+total-1)/total;
 if(target<0)target=0;if(target>length)target=length;
 int x=(32-length)/2;
 for(int y=row;y<row+1+big;y++)for(int i=0;i<32;i++)ram3000.pause_menu_map_tilemap[y*32+i]=0x207f;
 for(int i=0;i<target;i++,x++){
  int cp=sm_locale_next(&text),c=sm_locale_base(cp)-'A',tile=0x7f;
  if(c>=0&&c<26)tile=big?0x20+c+(c>=16?16:0):c;
  else if(cp=='\'')tile=big?0x4a:0x1d;
  if(big && tile!=0x7f)ram3000.pause_menu_map_tilemap[(row+1)*32+x]=0x2000|tile+16;
  if(cp==0xc0 && big){
   // Reuse an unused punctuation tile in this final caption, never the
   // adjacent map at 0x4800. This is the native four-plane ending font.
   uint16_t* accent=g_snes->ppu->vram+0x47e0;
   memcpy(accent,g_snes->ppu->vram+0x4200,32);
   accent[0]|=0x2020;accent[1]|=0x1010;
   accent[8]|=0x2020;accent[9]|=0x1010;tile=0x7e;
  }
  ram3000.pause_menu_map_tilemap[row*32+x]=(big?0x2000:0x3c00)|tile;
 }
 return 1;
}
void sm_locale_ending_roles(void){
 if(!sm_locale_get() || game_state!=39 || cinematic_function<0xe190 || cinematic_function>0xe314)return;
 for(int y=0;y<28;y++){
  char decoded[33];int palette=0;
  for(int x=0;x<32;x++){int t=ram3000.pause_menu_map_tilemap[y*32+x];int c=t&1023;decoded[x]=c<26?'A'+c:c==0x4f||c==0x7f?' ':'?';if(c<26)palette=t&0xfc00;}decoded[32]=0;
  char* text=decoded;while(*text==' ')text++;int end=strlen(text);while(end && text[end-1]==' ')text[--end]=0;
  const char* translated=!strcmp(text,"PRODUCED BY")?"UNE PRODUCTION DE":!strcmp(text,"OF")?"DE":NULL;
  if(!translated)continue;
  for(int x=0;x<32;x++)ram3000.pause_menu_map_tilemap[y*32+x]=0x7f;
  int x=(32-strlen(translated))/2;
  for(;*translated;translated++,x++)ram3000.pause_menu_map_tilemap[y*32+x]=palette|(*translated==' '?0x7f:*translated-'A');
 }
}
