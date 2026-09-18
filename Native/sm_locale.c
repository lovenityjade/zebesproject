#include "sm_locale.h"
#include <string.h>
static int language;
static const struct {const char *en,*fr;} catalog[]={
#include "sm_locale_catalog.inc"
};
void sm_locale_set(int value){language=value==1;}
int sm_locale_get(void){return language;}
const char *sm_locale_text(const char *text){
  if(!language || !text)return text;
  int lo=0,hi=sizeof(catalog)/sizeof(*catalog);
  while(lo<hi){int mid=(lo+hi)/2,c=strcmp(text,catalog[mid].en);if(!c)return catalog[mid].fr;if(c<0)hi=mid;else lo=mid+1;}
  return text;
}
int sm_locale_next(const char **text){
  const unsigned char *p=(const unsigned char*)*text;unsigned c=*p++;
  if(c>=0xc2 && c<=0xdf && (p[0]&0xc0)==0x80){c=((c&31)<<6)|(p[0]&63);p++;}
  else if(c>=0xe0 && c<=0xef && p[0] && p[1] && (p[0]&0xc0)==0x80 && (p[1]&0xc0)==0x80){c=((c&15)<<12)|((p[0]&63)<<6)|(p[1]&63);p+=2;}
  *text=(const char*)p;return c;
}
int sm_locale_length(const char *text){int n=0;while(*text){sm_locale_next(&text);n++;}return n;}
int sm_locale_base(int c){
  if(c>='a'&&c<='z')return c-32;
  switch(c){case 0xc0:case 0xe0:case 0xc2:case 0xe2:return 'A';case 0xc7:case 0xe7:return 'C';
  case 0xc8:case 0xe8:case 0xc9:case 0xe9:case 0xca:case 0xea:case 0xcb:case 0xeb:return 'E';
  case 0xce:case 0xee:case 0xcf:case 0xef:return 'I';case 0xd4:case 0xf4:return 'O';
  case 0xd9:case 0xf9:case 0xdb:case 0xfb:case 0xdc:case 0xfc:return 'U';case 0x2019:return '\'';}
  return c;
}
int sm_locale_accent(int c){
  switch(c){case 0xc9:case 0xe9:return 1;case 0xc0:case 0xe0:case 0xc8:case 0xe8:case 0xd9:case 0xf9:return 2;
  case 0xc2:case 0xe2:case 0xca:case 0xea:case 0xce:case 0xee:case 0xd4:case 0xf4:case 0xdb:case 0xfb:return 3;
  case 0xcb:case 0xeb:case 0xcf:case 0xef:case 0xdc:case 0xfc:return 4;case 0xc7:case 0xe7:return 5;default:return 0;}
}
