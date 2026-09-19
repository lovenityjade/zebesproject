#include "sm_locale.h"
#include "sm_dialogue.h"
#include "sm_rtl.h"
#include <math.h>
#include <string.h>

#define MAX_TEXT 4096
#define MAX_LINES 512
static unsigned char frame[8][8*8*4],font[128][64],arrow[16*8*4];
static unsigned char pixels[400*52*4];
static uint16_t lines[MAX_LINES][42];
static int loaded,active,width,line_count,page,visible,completed,held;
static float elapsed,blink;
static int pc(int a){return ((a>>16)&127)*32768+(a&32767);}
static int word(int a){return g_rom[pc(a)]|(g_rom[pc(a)+1]<<8);}
static int index4(const uint8_t *tile,int x,int y){
  int c=0;for(int p=0;p<4;p++)c|=((tile[y*2+p/2*16+p%2]>>(7-x))&1)<<p;return c;
}
static void color(uint8_t *p,int c){
  for(int i=0;i<3;i++){int v=(c>>((2-i)*5))&31;p[i]=(v<<3)|(v>>2);}p[3]=255;
}
void sm_dialogue_close(void){active=completed=0;held=1;}
void sm_dialogue_unload(void){sm_dialogue_close();loaded=0;page=line_count=visible=0;width=400;elapsed=blink=0;memset(pixels,0,sizeof(pixels));}
void sm_dialogue_load_assets(void){
  sm_dialogue_unload();
  // Exact crops used by the approved lab, mapped back to the original BG2.
  const int points[8][2]={{2,41},{32,41},{246,41},{2,64},{246,64},{2,189},{32,189},{246,189}};
  memset(frame,0,sizeof(frame));memset(font,0,sizeof(font));memset(arrow,0,sizeof(arrow));
  for(int n=0;n<8;n++)for(int y=0;y<8;y++)for(int x=0;x<8;x++){
    int sx=points[n][0]+x,sy=points[n][1]+y+1;
    int t=word(0xb6e000+((sy/8)*32+sx/8)*2);
    int tx=sx&7,ty=sy&7;if(t&0x4000)tx=7-tx;if(t&0x8000)ty=7-ty;
    int c=index4(g_rom+pc(0xb68000)+(t&1023)*32,tx,ty);
    int rgb=word(0xb6f000+2*(((t>>10)&7)*16+c));
    if((rgb&31)==((rgb>>5)&31) && (rgb&31)==((rgb>>10)&31))color(frame[n]+(y*8+x)*4,rgb);
  }
  for(int c='A';c<='Z';c++)for(int y=0;y<8;y++)for(int x=0;x<8;x++)
    font[c][y*8+x]=index4(g_rom+pc(0xb68000)+(0x30+c-'A')*32,x,y)==13;
  for(int c='0';c<='9';c++)for(int y=0;y<8;y++)for(int x=0;x<8;x++){
    const uint8_t *t=g_rom+pc(0x9ab200)+((c-'0'+9)%10)*16;
    font[c][y*8+x]=(((t[y*2]>>(7-x))&1)|(((t[y*2+1]>>(7-x))&1)<<1))==2;
  }
  const char punctuation[]=".?!";const int tiles[]={0x4a,0x4b,0x4c};
  for(int i=0;i<3;i++)for(int y=0;y<8;y++)for(int x=0;x<8;x++)
    font[(int)punctuation[i]][y*8+x]=index4(g_rom+pc(0xb68000)+tiles[i]*32,x,y)==13;
  for(int x=0;x<15;x++)for(int y=0;y<8;y++){
    int c=index4(g_rom+pc(0xb6c000)+0x9d*32,x<8?x:14-x,y);
    if(c)color(arrow+(y*16+x)*4,word(0xb6f000+2*(176+c)));
  }
  // Native item-message apostrophe and comma, not the pause '?' tile.
  const int marks[]={39,44},mark_tiles[]={0xfd,0xfb};
  for(int c=0;c<2;c++){
    const uint8_t* t=g_rom+pc(0x9ab200)+mark_tiles[c]*16;
    for(int y=0;y<8;y++)for(int x=0;x<8;x++)font[marks[c]][y*8+x]=(((t[y*2]>>(7-x))&1)|(((t[y*2+1]>>(7-x))&1)<<1))==1;
  }
  // Dash uses the same two-pixel stroke weight as the native punctuation.
  for(int y=3;y<5;y++)for(int x=1;x<7;x++)font['-'][y*8+x]=1;
  // Colon and semicolon use two of the native dot glyph's pixels.
  memset(font[':'],0,64);font[':'][2*8+3]=font[':'][5*8+3]=1;
  memcpy(font[';'],font[':'],64);font[';'][6*8+2]=1;
  loaded=1;
}
static int supported(int cp){int c=sm_locale_base(cp);return c==' ' || c=='-' || c=='\'' || c=='!' || c=='.' || c==',' || c==':' || c==';' || c=='?' || (c>='A'&&c<='Z') || (c>='0'&&c<='9');}
static int text_length(const uint16_t* s){int n=0;while(s[n])n++;return n;}
int sm_dialogue_open(const char *text,int w){
 if(!loaded || active || !text || (w!=256 && w!=400))return 0;
 size_t size=0;while(size<=MAX_TEXT && text[size])size++;
 if(!size || size>MAX_TEXT)return 0;
 // Decode before wrapping: a UTF-8 accent is one character and one reveal tick.
 uint16_t decoded[MAX_TEXT+1];int count=0,content=0;const char* cursor=text;
 while(*cursor){int cp=sm_locale_next(&cursor);if(cp!='\n' && cp!='\r' && !supported(cp))return 0;
  if(cp>='a'&&cp<='z')cp-=32;else if(cp>=0xe0&&cp<=0xfc && sm_locale_accent(cp))cp-=32;
  if(cp==0x2019)cp='\'';decoded[count++]=cp;if(cp!=' ' && cp!='\n' && cp!='\r')content=1;
 }
 if(!content)return 0;decoded[count]=0;
 uint16_t pending[MAX_LINES][42]={{0}};int row=0,col=0,limit=(w-80)/8;
 for(int i=0;i<count;){
  int c=decoded[i];if(c=='\r'){i++;continue;}if(c=='\n'){if(++row>=MAX_LINES)return 0;col=0;i++;continue;}
  if(c==' '){if(col && col<limit)pending[row][col++]=' ';i++;continue;}
  int end=i;while(end<count && decoded[end]!=' ' && decoded[end]!='\n' && decoded[end]!='\r')end++;
  if(col && end-i>limit-col){while(col && pending[row][col-1]==' ')pending[row][--col]=0;if(++row>=MAX_LINES)return 0;col=0;}
  while(i<end){if(col==limit){if(++row>=MAX_LINES)return 0;col=0;}pending[row][col++]=decoded[i++];}
 }
 for(int r=0;r<=row;r++){int n=text_length(pending[r]);while(n && pending[r][n-1]==' ')pending[r][--n]=0;}
 while(row && !pending[row][0])row--;
 memcpy(lines,pending,sizeof(lines));line_count=row+1;width=w;page=visible=completed=0;elapsed=blink=0;held=1;active=1;return 1;
}
static int count(void){int a=page*2;return text_length(lines[a])+(a+1<line_count?text_length(lines[a+1]):0);}
void sm_dialogue_tick(float seconds,int confirm){
  if(!active)return;
  if(!isfinite(seconds) || seconds<0)seconds=0;
  if(seconds>.1f)seconds=.1f;
  int press=confirm && !held;held=!!confirm;
  blink+=seconds;elapsed+=seconds;
  visible=(int)(elapsed/.032f);if(visible>count())visible=count();
  if(press){
    if(visible<count()){visible=count();elapsed=visible*.032f+.001f;}
    else if((page+1)*2<line_count){page++;visible=0;elapsed=blink=0;}
    else {active=0;completed=1;}
  }
}
/* Seek through every page automatically; reveal in the first 55% of each
 * page's share, leave the rest readable. Caller owns lifetime and timing. */
void sm_dialogue_auto(float progress){
 if(!active || !isfinite(progress))return;
 if(progress<0)progress=0;if(progress>1)progress=1;
 int pages=(line_count+1)/2;float position=progress*pages;
 page=(int)position;if(page>=pages)page=pages-1;
 float reveal=(position-page)/.55f;
 visible=(int)(count()*reveal);if(visible>count())visible=count();
 blink=.5f; /* automatic dialogue has no misleading confirm arrow */
}
int sm_dialogue_state(int field){
  switch(field){case 0:return active;case 1:return page;case 2:return (line_count+1)/2;
    case 3:return active && visible==count();case 4:return completed;case 5:return width;case 6:return visible;default:return 0;}
}
static void copy(int x,int y,const uint8_t *p){if(p[3])memcpy(pixels+(y*width+x)*4,p,4);}
static void panel(int left,int w){
  for(int y=0;y<52;y++)for(int x=0;x<w;x++){
    int part=-1,sx=x&7,sy=y&7;
    if(y<8){part=x<8?0:x>=w-8?2:1;if(x>=w-8)sx=x-w+8;}
    else if(y>=44){part=x<8?5:x>=w-8?7:6;sy=y-44;if(x>=w-8)sx=x-w+8;}
    else if(x<8)part=3;else if(x>=w-8){part=4;sx=x-w+8;}
    if(part>=0)copy(left+x,y,frame[part]+(sy*8+sx)*4);
  }
}
const uint8_t *sm_dialogue_pixels(void){
  memset(pixels,0,sizeof(pixels));if(!active)return pixels;
  for(int i=0;i<width*52;i++)pixels[i*4+3]=255;
  panel(0,52);panel(56,width-56);
  int remaining=visible;
  for(int r=0;r<2 && page*2+r<line_count;r++){
    const uint16_t *s=lines[page*2+r];int length=text_length(s),n=remaining<length?remaining:length;
    for(int i=0;i<n;i++)for(int y=0;y<8;y++)for(int x=0;x<8;x++)if(font[sm_locale_base(s[i])][y*8+x]){
      uint8_t *p=pixels+((16+r*11+y)*width+68+i*8+x)*4;memset(p,255,4);
    }
    for(int i=0;i<n;i++){
      int a=sm_locale_accent(s[i]),x=68+i*8,y=14+r*11;
      static const uint8_t masks[5][2]={{8,16},{32,16},{16,40},{0,36},{0,16}};
      if(a)for(int dy=0;dy<2;dy++)for(int dx=0;dx<8;dx++)if(masks[a-1][dy]&(1<<(7-dx)))memset(pixels+(((a==5?y+10:y)+dy)*width+x+dx)*4,255,4);
    }
    remaining-=n;
  }
  if(visible==count() && ((int)(blink/.5f)&1)==0)
    for(int y=0;y<8;y++)for(int x=0;x<16;x++)copy(width-26+x,35+y,arrow+(y*16+x)*4);
  return pixels;
}

void sm_native_frame(uint8_t *out,int w,int h){
 if(!loaded || w<16 || h<16)return;
 for(int y=0;y<h;y++)for(int x=0;x<w;x++){
  uint8_t *dst=out+(y*w+x)*4;dst[0]=dst[1]=dst[2]=0;dst[3]=255;
  int part=-1,sx=x&7,sy=y&7;
  if(y<8){part=x<8?0:x>=w-8?2:1;if(x>=w-8)sx=x-w+8;}
  else if(y>=h-8){part=x<8?5:x>=w-8?7:6;sy=y-h+8;if(x>=w-8)sx=x-w+8;}
  else if(x<8)part=3;else if(x>=w-8){part=4;sx=x-w+8;}
  if(part>=0 && frame[part][(sy*8+sx)*4+3])memcpy(dst,frame[part]+(sy*8+sx)*4,4);
 }
}
void sm_native_menu_cursor(uint8_t *out,int w,int h,int x,int y){
 if(!loaded)return;
 /* Rotate the native map's downward arrow toward the selected row. */
 for(int sy=0;sy<8;sy++)for(int sx=0;sx<15;sx++){
  int dx=x+sy,dy=y+14-sx;const uint8_t *p=arrow+(sy*16+sx)*4;
  if(dx>=0 && dx<w && dy>=0 && dy<h && p[3])memcpy(out+(dy*w+dx)*4,p,4);
 }
}
