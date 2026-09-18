#include "sm_locale.h"
#include "sm_cpu_infra.h"
#include "sm_rtl.h"
#include "ida_types.h"
#include "variables.h"
#include <string.h>
static const int accents[]={0xc0,0xc2,0xc7,0xc8,0xc9,0xca,0xcb,0xce,0xcf,0xd4,0xd9,0xdb,0xdc};
static const int punctuation[]={39,33,44,63,46};
static const uint8_t tops[26]={0x0a,0x0b,0x0c,0x0d,0x0e,0x0e,0x0c,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x00,0x0d,0x00,0x0d,0x2b,0x2c,0x2d,0x2d,0x2d,0x40,0x41,0x42};
static const uint8_t bots[26]={0x1a,0x1b,0x1c,0x1d,0x1e,0x25,0x30,0x31,0x11,0x33,0x34,0x35,0x36,0x37,0x10,0x38,0x39,0x3a,0x3b,0x11,0x3d,0x3e,0x3f,0x50,0x11,0x52};
static void accent(uint8_t* p,int a){
 int mask[2]={0,0};if(a==1){mask[0]=8;mask[1]=16;}if(a==2){mask[0]=32;mask[1]=16;}
 if(a==3){mask[0]=16;mask[1]=40;}if(a==4)mask[1]=36;
 for(int y=0;y<2;y++){p[y*2]&=~mask[y];p[y*2+1]|=mask[y];p[y*2+16]|=mask[y];p[y*2+17]|=mask[y];}
}
/* Menu graphics occupy 0000..2aff; 2b00..2fff is the gap before the
 * separately loaded pause atlas at 3000. This pool is menu-only and gets
 * discarded when native gameplay reloads its VRAM. */
void sm_locale_menu_font(void){
 if(!sm_locale_get() || (game_state!=2 && game_state!=4))return;
 const uint8_t* font=RomFixedPtr(0x9ab200);
 for(int i=0;i<54;i++){
  int cp=i<26?'A'+i:i<36?'0'+i-26:i<41?punctuation[i-36]:accents[i-41];
  int c=sm_locale_base(cp),a=sm_locale_accent(cp),tile;
  if(c>='A'&&c<='Z')tile=0xe0+c-'A';else if(c>='0'&&c<='9')tile=(c-'0'+9)%10;else tile=(int[]){0xfd,0xff,0xfb,0xfe,0xfc}[i-36];
  uint8_t* dst=(uint8_t*)(g_snes->ppu->vram+(0x2b0+i)*16);memset(dst,0,32);
  for(int y=0;y<6;y++)for(int x=0;x<8;x++){
   const uint8_t* p=font+tile*16;int c=((p[y*2]>>(7-x))&1)|(((p[y*2+1]>>(7-x))&1)<<1);
   if(c==1){int bit=1<<(7-x);dst[(y+2)*2+1]|=bit;dst[(y+2)*2+16]|=bit;dst[(y+2)*2+17]|=bit;}
  }
  accent(dst,a);
 }
 for(int i=0;i<13;i++){
  uint8_t* dst=(uint8_t*)(g_snes->ppu->vram+(0x2e6+i)*16);
  memcpy(dst,RomFixedPtr(0x8e8000)+tops[sm_locale_base(accents[i])-'A']*32,32);accent(dst,sm_locale_accent(accents[i]));
 }
}
uint16_t sm_locale_menu_small(int cp){
 if(cp>='a'&&cp<='z')cp-=32;
 if(cp>='A'&&cp<='Z')return 0x2b0+cp-'A';if(cp>='0'&&cp<='9')return 0x2b0+26+cp-'0';
 if(cp>=0xe0&&cp<=0xfc)cp-=32;
 for(int i=0;i<5;i++)if(cp==punctuation[i] || (cp==0x2019&&i==0))return 0x2b0+36+i;
 for(int i=0;i<13;i++)if(cp==accents[i])return 0x2b0+41+i;
 if(cp=='-')return 0x87;if(cp==':')return 0x8c;return 0x0f;
}
void sm_locale_menu_big(int cp,uint16_t* top,uint16_t* bottom){
 int c=sm_locale_base(cp);*top=*bottom=15;
 if(c<'A'||c>'Z')return;*top=tops[c-'A'];*bottom=bots[c-'A'];
 if(!sm_locale_get())return;if(cp>=0xe0&&cp<=0xfc)cp-=32;
 for(int i=0;i<13;i++)if(cp==accents[i])*top=0x2e6+i;
}
int sm_locale_menu_tilemap(unsigned k,unsigned j){
 if(!sm_locale_get() || game_state!=4)return 0;
 const char* label=0;int big=0;
 switch(j){
 case addr_kMenuTilemap_SamusData:label="SAUVEGARDES";big=1;break;
 case addr_kMenuTilemap_Energy:label="ÉN.";break;
 case addr_kMenuTilemap_TIME:label="TPS";break;
 case addr_kMenuTilemap_NoData:label=" VIDE";break;
 case addr_kMenuTilemap_DataCopy:label="COPIER";break;
 case addr_kMenuTilemap_DataClear:label="EFFACER";break;
 case addr_kMenuTilemap_Exit:label="RETOUR";break;
 case addr_kMenuTilemap_DataCopyMode:label="COPIER DONNÉES";big=1;break;
 case addr_kMenuTilemap_DataClearMode:label="EFFACER DONNÉES";big=1;break;
 case addr_kMenuTilemap_CopyWhichData:label="QUELLE PARTIE COPIER?";break;
 // Native routines insert A/B/C at these exact columns after this call.
 case addr_kMenuTilemap_CopySamusToWhere:label="COPIE SAMUS A VERS ?";break;
 case addr_kMenuTilemap_CopySamusToSamus:label="COPIE SAMUS A VERS SAMUS B";break;
 case addr_kMenuTilemap_IsThisOk:label="CONFIRMER?";break;
 case addr_kMenuTilemap_Yes:label="OUI";big=1;break;
 case addr_kMenuTilemap_No:label="NON";big=1;break;
 case addr_kMenuTilemap_CopyCompleted:label="COPIE TERMINÉE";break;
 case addr_kMenuTilemap_ClearWhichData:label="QUELLE PARTIE EFFACER?";break;
 case addr_kMenuTilemap_ClearSamusA:label="SUPPRIMER LA PARTIE  A ?";break;
 case addr_kMenuTilemap_DataCleared:label="SAUVEGARDE EFFACÉE";break;
 default:return 0;
 }
 sm_locale_menu_font();
 unsigned offset=k/2,col=offset%32,row=offset/32;
 // Clear only the original resource's cells; adjacent slot data stays intact.
 unsigned src=j,cursor=offset,start=offset;for(int n=0;n<128;n++,src+=2){unsigned address=0x810000|src;unsigned t=GET_WORD(RomFixedPtr(address));if(t==0xffff)break;if(t==0xfffe){cursor=(start+=32);continue;}if(cursor<1024)ram3000.pause_menu_map_tilemap[768+cursor]=15;cursor++;}
 for(;*label && col<32;col++){
  int cp=sm_locale_next(&label);uint16_t top=sm_locale_menu_small(cp),bottom=15;
  if(big)sm_locale_menu_big(cp,&top,&bottom);
  ram3000.pause_menu_map_tilemap[768+row*32+col]=0x2000|enemy_data[0].palette_index|top;
  if(big && row<31)ram3000.pause_menu_map_tilemap[768+(row+1)*32+col]=0x2000|enemy_data[0].palette_index|bottom;
 }
 return 1;
}
