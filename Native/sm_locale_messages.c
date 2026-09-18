#include "sm_locale.h"
#include "sm_relic.h"
#include "ida_types.h"
#include "variables.h"
#include <stdio.h>
#include <string.h>
#include "sm_cpu_infra.h"
#include "sm_rtl.h"
/* The message font lives in BG3's dedicated HUD bank. Borrow 64 unused
 * Japanese-font tiles while the box is open, and restore the exact bytes on
 * close. Letters are copied from the verified ROM, never distributed. */
static uint16_t saved_font[64*8];
static int font_active;
void sm_locale_reset(void){font_active=0;}
static const int accents[]={0xc0,0xc2,0xc7,0xc8,0xc9,0xca,0xcb,0xce,0xcf,0xd4,0xd9,0xdb,0xdc};
static const int punctuation[]={39,33,44,63,46};
static int slot(int cp){
 if(cp>='a'&&cp<='z')cp-=32;
 if(cp>='A'&&cp<='Z')return cp-'A';
 if(cp>='0'&&cp<='9')return 26+cp-'0';
 if(cp>=0xe0&&cp<=0xfc)cp-=32;
 for(int i=0;i<5;i++)if(cp==punctuation[i] || (cp==0x2019 && i==0))return 36+i;
 for(int i=0;i<13;i++)if(cp==accents[i])return 41+i;
 return -1;
}
uint16_t sm_locale_message_glyph(int cp){int i=slot(cp);return i<0?0x284e:0x2980+i;}
void sm_locale_message_font(void){
 if(!sm_locale_get()||font_active)return;
 memcpy(saved_font,g_snes->ppu->vram+0x4c00,sizeof(saved_font));font_active=1;
 for(int i=0;i<54;i++){
  int cp=i<26?'A'+i:i<36?'0'+i-26:i<41?punctuation[i-36]:accents[i-41];
  int base=sm_locale_base(cp),a=sm_locale_accent(cp),tile;
  if(base>='A'&&base<='Z')tile=0xe0+base-'A';
  else if(base>='0'&&base<='9')tile=(base-'0'+9)%10;
  else tile=(int[]){0xfd,0xff,0xfb,0xfe,0xfc}[i-36];
  const uint8_t* p=RomFixedPtr(0x9ab200)+tile*16;
  uint16_t* dst=g_snes->ppu->vram+0x4c00+i*8;
  for(int y=0;y<8;y++)dst[y]=y<2?0xffff:GET_WORD(p+(y-2)*2);
  // Native foreground is color 1 against color 3. Accent pixels clear plane 1.
  if(a==1){dst[0]&=~(1u<<(8+3));dst[1]&=~(1u<<(8+4));}
  if(a==2){dst[0]&=~(1u<<(8+5));dst[1]&=~(1u<<(8+4));}
  if(a==3){dst[0]&=~(1u<<(8+4));dst[1]&=~((1u<<(8+5))|(1u<<(8+3)));}
  if(a==4)dst[1]&=~((1u<<(8+5))|(1u<<(8+2)));
  if(a==5){dst[7]&=~(1u<<(8+4));}
 }
}
void sm_locale_message_restore(void){if(font_active){memcpy(g_snes->ppu->vram+0x4c00,saved_font,sizeof(saved_font));font_active=0;}}
static void line(uint16_t* row,const char* text){
 int x=(32-sm_locale_length(text))/2;
 while(*text && x<29){uint16_t t=sm_locale_message_glyph(sm_locale_next(&text));if(x>=3)row[x]=t;x++;}
}
void sm_locale_message_choices(uint16_t* row){
 if(!sm_locale_get())return;
 // Preserve selection, control flow and cursor tiles; only replace the words.
 for(int i=10;i<=12;i++)row[i]=(save_confirmation_selection?0x2c00:0x3c00)+(sm_locale_message_glyph("OUI"[i-10])&1023);
 for(int i=19;i<=21;i++)row[i]=(save_confirmation_selection?0x3c00:0x2c00)+(sm_locale_message_glyph("NON"[i-19])&1023);
}
void sm_locale_message_tiles(uint16_t* body,int index){
 for(int y=0;y<4;y++)for(int x=0;x<32;x++)body[y*32+x]=x<3||x>=29?0x000e:0x284e;
 static const char* names[]={"","RÉSERVE D'ÉNERGIE","MISSILE","SUPER MISSILE","BOMBE DE PUISSANCE","RAYON GRAPPIN","VISEUR RADIOSCOPIQUE","COMBINAISON VARIA","BOULE DE SAUT","MORPHOSPHÈRE","ATTAQUE EN VRILLE","BOTTES DE SAUT","SAUT SPATIAL","ACCÉLÉRATEUR","RAYON DE CHARGE","RAYON DE GLACE","RAYON À ONDES","RAYON SPAZER","RAYON PLASMA","BOMBES","DONNÉES DE CARTE","ÉNERGIE RÉTABLIE","MISSILES RECHARGÉS","SAUVEGARDER?","SAUVEGARDE TERMINÉE","RÉSERVE AUXILIAIRE","COMBINAISON DE GRAVITÉ","","SAUVEGARDER?"};
 if(index<=0||index>=29)return;
 line(body,names[index]);
 if(index==23||index==28){body[96+8]=0x38cc;body[96+9]=0x38cd;sm_locale_message_choices(body+96);return;}
 if(index==20){line(body+64,"TRANSFERT TERMINÉ");return;}
 const char* tip=NULL;
 switch(index){case 2:case 3:case 4:case 5:case 6:tip="CHOISIR AVEC SELECT";break;
 case 8:tip="SAUTER EN MORPHOSPHÈRE";break;case 9:tip="APPUYER DEUX FOIS SUR BAS";break;
 case 10:tip="SAUTER EN TOURNANT";break;case 12:tip="SAUTER À RÉPÉTITION";break;
 case 13:tip="MAINTENIR COURSE";break;case 14:tip="MAINTENIR TIR";break;case 19:tip="TIRER EN MORPHOSPHÈRE";break;}
 if(tip)line(body+64,tip);
}
