#include "sm_route.h"
#include "sm_seed.h"
#include "sm_generation.h"
#include "sm_map_browser.h"
#include "ida_types.h"
#include "variables.h"
#include "sm_rtl.h"
#include "sm_profile.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
typedef struct {uint32_t frame;uint16_t room,x,y;uint8_t area,break_path;} Point;
typedef struct {char magic[8],seed[65];uint8_t complete,partial;uint32_t count;} Header;
static Point *points;static unsigned count,capacity,written;static Header header;
static char prefix[4096],path[4240];static int slot=-1,gap=1,cursor,last_area=-1,last_cursor=-1,failed;
static uint8_t background[528*320*4],trail[528*320*4],pixels[528*320*4];
static uint32_t clock_frame;
static int fresh;
static int archive_route(const char *file){
  FILE *f=fopen(file,"rb");if(!f)return errno==ENOENT;
  fclose(f);char archive[4300];
  snprintf(archive,sizeof(archive),"%s.archive-%llu",file,(unsigned long long)sm_clock_ns());
  return rename(file,archive)==0;
}
static void route_path(char *out,size_t size,int index,const char *seed){
  snprintf(out,size,"%s.route-%d-%s",prefix,index,seed);
}
/* A copied save inherits its source history; an overwritten target is archived.
 * Capture seed identities before the profile callback changes the slot plans. */
void sm_route_slot_action(int action,int from,int to,const char *source_seed,const char *target_seed){
  if(action!=0 && action!=2 && action!=3 && action!=4)return;
  int target=action==2?to:from;if(target<0 || target>2 || !prefix[0])return;
  if(!sm_route_save())return;if(slot==target)sm_route_close();
  char destination[4240];
  if(strlen(target_seed)==64){
    route_path(destination,sizeof(destination),target,target_seed);
    if(!archive_route(destination)){failed=1;return;}
  }
  if(action!=2 || strlen(source_seed)!=64)return;
  route_path(destination,sizeof(destination),target,source_seed);
  if(strcmp(source_seed,target_seed) && !archive_route(destination)){failed=1;return;}
  char source[4240],temp[4250];route_path(source,sizeof(source),from,source_seed);
  FILE *in=fopen(source,"rb");if(!in){if(errno!=ENOENT)failed=1;return;}
  snprintf(temp,sizeof(temp),"%s.tmp",destination);FILE *out=fopen(temp,"wb");
  if(!out){fclose(in);failed=1;return;}
  unsigned char buffer[16384];size_t n;int ok=1;
  while((n=fread(buffer,1,sizeof(buffer),in))!=0)if(fwrite(buffer,1,n,out)!=n){ok=0;break;}
  if(ferror(in))ok=0;fclose(in);if(fclose(out))ok=0;
  if(ok && !rename(temp,destination))return;
  remove(temp);failed=1;
}
void sm_route_new_game(void){sm_route_close();fresh=1;}
int sm_route_save(void){
  if(!path[0]||failed)return !failed;
  FILE *f=fopen(path,written?"r+b":"w+b");if(!f){failed=1;return 0;}
  header.count=count;int ok=1;
  if(ok)ok=fseek(f,sizeof(header)+(long)written*sizeof(Point),SEEK_SET)==0;
  if(ok && count>written)ok=fwrite(points+written,sizeof(Point),count-written,f)==count-written;
  if(ok)ok=fflush(f)==0 && fseek(f,0,SEEK_SET)==0;
  if(ok)ok=fwrite(&header,1,sizeof(header),f)==sizeof(header);
  if(fclose(f))ok=0;if(ok)written=count;else failed=1;return ok;
}
void sm_route_close(void){sm_route_save();free(points);points=0;count=capacity=written=0;path[0]=0;slot=-1;last_area=last_cursor=-1;failed=0;}
void sm_route_open(const char *save){sm_route_close();snprintf(prefix,sizeof(prefix),"%s",save);gap=1;clock_frame=0;fresh=0;}
void sm_route_session(void){gap=1;}
static void select_route(void){
  int s=sm_slots_current();if(s<0)s=selected_save_slot;
  if(s==slot && !strcmp(header.seed,sm_seed_fingerprint()))return;
  sm_route_close();slot=s;memset(&header,0,sizeof(header));memcpy(header.magic,"ZBROUTE1",8);snprintf(header.seed,sizeof(header.seed),"%s",sm_seed_fingerprint());
  header.partial=game_time_hours||game_time_minutes||game_time_seconds>5;
  snprintf(path,sizeof(path),"%s.route-%d-%s",prefix,slot,header.seed);
  if(fresh){
    if(!archive_route(path)){failed=1;return;}
    fresh=0;header.partial=0;
  }
  FILE *f=fopen(path,"rb");if(f){
    Header h;int ok=fread(&h,1,sizeof(h),f)==sizeof(h)&&!memcmp(h.magic,header.magic,8)&&h.seed[64]==0&&!strcmp(h.seed,header.seed)&&h.complete<=1&&h.partial<=1&&h.count<=5000000;
    if(ok){points=malloc((size_t)h.count*sizeof(Point));ok=!h.count||points;}
    if(ok){ok=fread(points,sizeof(Point),h.count,f)==h.count;for(unsigned i=0;ok&&i<h.count;i++)ok=points[i].area<6&&points[i].x<16384&&points[i].y<8192;}
    fclose(f);if(ok){header=h;count=capacity=written=h.count;clock_frame=count?points[count-1].frame+1:0;}else{free(points);points=0;failed=1;}
  }
  gap=1;
}
void sm_route_frame(void){
  if(!sm_seed_active() || game_state!=8 || sm_message_active() || area_index>=6)return;
  select_route();if(failed||header.complete)return;clock_frame++;
  Point p={clock_frame,room_ptr,(uint16_t)(room_x_coordinate_on_map*256+samus_x_pos),(uint16_t)((room_y_coordinate_on_map+1)*256+samus_y_pos),(uint8_t)area_index,(uint8_t)gap};
  if(p.x>=16384||p.y>=8192)return;
  Point *last=count?&points[count-1]:0;
  if(last && !gap && p.room==last->room && p.area==last->area &&
      ((p.x==last->x&&p.y==last->y)||p.frame-last->frame<6))return;
  if(count==capacity){unsigned n=capacity?capacity*2:4096;if(n>5000000)n=5000000;if(n==capacity){failed=1;return;}Point *q=realloc(points,n*sizeof(Point));if(!q){failed=1;return;}points=q;capacity=n;}
  points[count++]=p;gap=0;if(count-written>=128)sm_route_save();
}
void sm_route_finish(void){if(sm_seed_active()&&path[0]){header.complete=1;sm_route_save();}}
int sm_route_state(int field){switch(field){case 0:return count;case 1:return header.complete;case 2:return header.partial;case 3:return cursor;case 4:return failed;default:return 0;}}
static void put(uint8_t *out,int x,int y,uint32_t rgb){if(x<0||x>=528||y<0||y>=320)return;uint8_t *p=out+(y*528+x)*4;p[0]=rgb;p[1]=rgb>>8;p[2]=rgb>>16;p[3]=255;}
static void base(int area){
  memset(background,0,sizeof(background));for(unsigned i=3;i<sizeof(background);i+=4)background[i]=255;
  const uint8_t *ptr=RomFixedPtr(0x82964a)+area*3;
  const uint16_t *map=(const uint16_t*)RomPtr(ptr[0]|ptr[1]<<8|ptr[2]<<16);
  const uint8_t *known=sm_seed_map_data(area),*gfx=RomFixedPtr(0xb68000);
  const uint16_t *palette=(const uint16_t*)RomFixedPtr(0xb6f000);
  for(int y=0;y<32;y++)for(int x=0;x<64;x++){
    int bit=((x&31)>>3)+4*((x&32)+y),mask=0x80>>(x&7);
    uint16_t tile=known[bit]&mask?map[(x&31)+y*32+(x&32)*32]:31;
    const uint8_t *t=gfx+(tile&1023)*32;
    for(int dy=0;dy<8;dy++)for(int dx=0;dx<8;dx++){
      int row=tile&0x8000?7-dy:dy,col=tile&0x4000?dx:7-dx,index=0;
      for(int p=0;p<4;p++)index|=((t[row*2+p/2*16+p%2]>>col)&1)<<p;
      uint16_t c=palette[((tile>>10)&7)*16+index];uint32_t rgb=((c&31)*255/31)<<16|(((c>>5)&31)*255/31)<<8|((c>>10)&31)*255/31;
      put(background,x*8+8+dx,y*8+32+dy,rgb);
    }
  }
  for(int x=6;x<522;x++){put(background,x,30,0xb0b8c0);put(background,x,289,0xb0b8c0);}
  for(int y=30;y<=289;y++){put(background,6,y,0xb0b8c0);put(background,521,y,0xb0b8c0);}
}
static void line(int x0,int y0,int x1,int y1){
  int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy;
  for(;;){put(trail,x0,y0,0x41d6bf);if(x0==x1&&y0==y1)break;int e=2*err;if(e>=dy){err+=dy;x0+=sx;}if(e<=dx){err+=dx;y0+=sy;}}
}
const uint8_t *sm_route_pixels(int at){
  if(!count)return 0;cursor=at<0?0:(unsigned)at>=count?(int)count-1:at;
  Point *p=&points[cursor];int area=p->area;
  if(area!=last_area || cursor<last_cursor){base(area);memcpy(trail,background,sizeof(trail));last_area=area;last_cursor=-1;}
  for(int i=last_cursor+1;i<=cursor;i++){
    Point *b=&points[i];if(b->area!=area)continue;
    int x=b->x/32+8,y=b->y/32+32;put(trail,x,y,0x41d6bf);
    if(i){Point *a=&points[i-1];if(!b->break_path&&a->area==area && abs((int)a->x-b->x)+abs((int)a->y-b->y)<768)line(a->x/32+8,a->y/32+32,x,y);}
  }
  last_cursor=cursor;memcpy(pixels,trail,sizeof(pixels));
  int x=p->x/32+8,y=p->y/32+32;for(int d=-3;d<=3;d++){put(pixels,x+d,y,0xffdf30);put(pixels,x,y+d,0xffdf30);}
  static const char *names[]={"CRATERIA","BRINSTAR","NORFAIR","WRECKED SHIP","MARIDIA","TOURIAN"};
  sm_native_text_height(pixels,528,320,8,8,"RUN RECAP",0xffffff);
  sm_native_text_height(pixels,528,320,232,8,names[area],0xffcf45);
  char label[48];snprintf(label,sizeof(label),"%d OF %u",cursor+1,count);sm_native_text_height(pixels,528,320,392,8,label,0xffffff);
  sm_native_text_height(pixels,528,320,8,299,"A PLAY PAUSE   LEFT RIGHT SEEK   L R SPEED   B BACK",0xb0d8ff);
  if(header.partial)sm_native_text_height(pixels,528,320,8,19,"PARTIAL HISTORY",0xffaa55);
  return pixels;
}
