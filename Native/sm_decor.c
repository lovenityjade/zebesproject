#include "sm_bridge.h"
#include "ida_types.h"
#include "variables.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

/* Presentation-only data. Never written into ROM, level/BTS data, RAM or SRAM. */
#define DECOR_CAPACITY 131072
#define SCENE_CAPACITY 1024
static struct DecorCell {
  uint16_t room,state,tile;int16_t x,y;
  uint8_t layer,tileset,used,secret,custom,opacity;uint16_t radius;
  int expected;
} decor[DECOR_CAPACITY];
static uint8_t decor_states[65536];
static unsigned decor_count;
struct DecorPlane {int enabled,sx,sy,ox,oy,repeat;char path[2048];};
struct DecorScene {int room,state;char atlas[2048];struct DecorPlane planes[4];};
static struct DecorScene scenes[SCENE_CAPACITY];static int scene_count;
struct DecorImage {uint8_t *data;int w,h;};
static struct DecorImage images[5];
static struct DecorScene *current;
static int current_room=-1,current_state=-1;
static void release_images(void){for(int i=0;i<5;i++){free(images[i].data);memset(&images[i],0,sizeof(images[i]));}}
static unsigned decor_hash(int room,int state,int layer,int x,int y) {
 return ((unsigned)room*31337u^(unsigned)state*83492791u^(unsigned)x*73856093u^(unsigned)y*19349663u^(unsigned)layer*2654435761u)&(DECOR_CAPACITY-1);
}
void sm_decor_clear(void){release_images();memset(decor,0,sizeof(decor));memset(decor_states,0,sizeof(decor_states));memset(scenes,0,sizeof(scenes));decor_count=scene_count=0;current=NULL;current_room=current_state=-1;}
int sm_decor_set_ex(int room,int state,int tileset,int layer,int x,int y,int tile,int expected,int secret,int radius,int opacity,int custom){
 if(room<0x8000||room>0xffff||state<0x8000||state>0xffff||tileset<0||tileset>28||layer<0||layer>1||x<-128||x>511||y<-128||y>511||tile<0||tile>4095||expected<-1||expected>65535||secret<0||secret>1||custom<0||custom>1||radius<16||radius>256||opacity<0||opacity>100||(secret&&layer))return 0;
 unsigned slot=decor_hash(room,state,layer,x,y);
 for(unsigned n=0;n<DECOR_CAPACITY;n++,slot=(slot+1)&(DECOR_CAPACITY-1)){
  struct DecorCell *c=&decor[slot];
  if(!c->used){if(decor_count>=100000)return 0;++decor_count;}
  else if(c->room!=room||c->state!=state||c->layer!=layer||c->x!=x||c->y!=y)continue;
  *c=(struct DecorCell){.room=room,.state=state,.tile=tile,.x=x,.y=y,.layer=layer,.tileset=tileset,.used=1,.secret=secret,.custom=custom,.opacity=opacity,.radius=radius,.expected=expected};
  decor_states[state]=1;return 1;
 }return 0;
}
int sm_decor_set(int room,int state,int tileset,int layer,int x,int y,int tile,int expected){return sm_decor_set_ex(room,state,tileset,layer,x,y,tile,expected,0,64,25,0);}
static const struct DecorCell *find_cell(int layer,int x,int y){
 if(!decor_states[roomdefroomstate_ptr])return NULL;
 unsigned slot=decor_hash(room_ptr,roomdefroomstate_ptr,layer,x,y);
 for(unsigned n=0;n<DECOR_CAPACITY;n++,slot=(slot+1)&(DECOR_CAPACITY-1)){
  const struct DecorCell *c=&decor[slot];if(!c->used)return NULL;
  if(c->room!=room_ptr||c->state!=roomdefroomstate_ptr||c->layer!=layer||c->x!=x||c->y!=y)continue;
  if(c->tileset!=get_RoomDefRoomstate(roomdefroomstate_ptr)->graphics_set)return NULL;
  if(c->expected>=0&&x>=0&&y>=0&&x<room_width_in_blocks&&y<room_height_in_blocks){
   int a=0x10002+2*(y*room_width_in_blocks+x)+(layer?0x9600:0);
   if(a<0||a+1>=0x20000||GET_WORD(g_ram+a)!=c->expected)return NULL;
  }return c;
 }return NULL;
}
int sm_decor_room_active(void){return decor_states[roomdefroomstate_ptr]!=0;}
int sm_decor_get(int layer,int x,int y,uint16_t *tile){const struct DecorCell *c=find_cell(layer,x,y);if(!c||c->secret||c->custom)return 0;*tile=c->tile;return 1;}
static struct DecorScene *scene(int room,int state){
 if(room<0x8000||room>0xffff||state<0x8000||state>0xffff)return NULL;
 for(int i=0;i<scene_count;i++)if(scenes[i].room==room&&scenes[i].state==state)return &scenes[i];
 if(scene_count==SCENE_CAPACITY)return NULL;
 struct DecorScene *s=&scenes[scene_count++];s->room=room;s->state=state;return s;
}
int sm_decor_set_plane(int room,int state,int id,int sx,int sy,int ox,int oy,int repeat,const char *path){
 if(id<-1||id>2||sx<0||sx>200||sy<0||sy>200||ox<-8192||ox>8192||oy<-8192||oy>8192||repeat<0||repeat>3||!path||strlen(path)>=2048)return 0;
 struct DecorScene *s=scene(room,state);if(!s)return 0;
 struct DecorPlane *p=&s->planes[id+1];*p=(struct DecorPlane){.enabled=1,.sx=sx,.sy=sy,.ox=ox,.oy=oy,.repeat=repeat};strcpy(p->path,path);decor_states[state]=1;current_room=-1;return 1;
}
int sm_decor_set_atlas(int room,int state,const char *path){
 if(!path||strlen(path)>=2048)return 0;struct DecorScene *s=scene(room,state);if(!s)return 0;
 strcpy(s->atlas,path);decor_states[state]=1;current_room=-1;return 1;
}
static unsigned le32(const uint8_t *b){return b[0]|(unsigned)b[1]<<8|(unsigned)b[2]<<16|(unsigned)b[3]<<24;}
#include "sm_decor_rom.h"
static void load_image(struct DecorImage *image,const char *path,int atlas){
 if(!*path)return;FILE *f=fopen(path,"rb");if(!f){fprintf(stderr,"SM_DECOR_IMAGE missing: %s\n",path);return;}
 uint8_t h[12];unsigned w=0,ht=0;uint8_t *data=NULL;
 if(fread(h,1,4,f)!=4)goto done;
 if(!memcmp(h,"ZDR1",4)){if(atlas)decor_rom_image(f,image);goto done;}
 if(memcmp(h,"ZBG1",4)||fread(h+4,1,8,f)!=8)goto done;
 w=le32(h+4);ht=le32(h+8);
 if(!w||!ht||w>4096||ht>4096||w*ht>4194304||(atlas&&(w%16||ht%16||w*ht>262144)))goto done;
 data=malloc(w*ht*4);if(!data)goto done;
 if(fread(data,1,w*ht*4,f)!=w*ht*4||fgetc(f)!=EOF){free(data);data=NULL;goto done;}
 image->data=data;image->w=w;image->h=ht;
 done:fclose(f);if(!image->data)fprintf(stderr,"SM_DECOR_IMAGE invalid: %s\n",path);
}
void sm_decor_prepare(void){
 if(current_room==room_ptr&&current_state==roomdefroomstate_ptr)return;
 release_images();current=NULL;current_room=room_ptr;current_state=roomdefroomstate_ptr;
 for(int i=0;i<scene_count;i++)if(scenes[i].room==room_ptr&&scenes[i].state==roomdefroomstate_ptr){current=&scenes[i];break;}
 if(!current)return;
 for(int i=0;i<4;i++)if(current->planes[i].enabled)load_image(&images[i],current->planes[i].path,0);
 load_image(&images[4],current->atlas,1);
}
static int plane_index(int wy){return room_ptr==0x91f8?(wy<1168?1:(wy<1192?2:3)):1;}
static int active_plane(int wy){if(!current)return -1;return current->planes[0].enabled?0:plane_index(wy);}
int sm_decor_parallax(int axis,int wy,int value){int i=active_plane(wy);if(i<0||!current->planes[i].enabled)return value;return value*(axis?current->planes[i].sy:current->planes[i].sx)/100;}
static const uint8_t *image_pixel(struct DecorImage *image,int x,int y,int repeat){
 if(!image->data)return NULL;
 if(repeat&1)x=(x%image->w+image->w)%image->w;
 if(repeat&2)y=(y%image->h+image->h)%image->h;
 if(x<0||y<0||x>=image->w||y>=image->h)return NULL;
 return image->data+4*(y*image->w+x);
}
static void blend(uint8_t *out,const uint8_t *p,int alpha,int brightness){
 if(!p)return;alpha=alpha*p[3]/255;
 for(int k=0;k<3;k++)out[k]=(out[k]*(255-alpha)+(p[k]*brightness/15)*alpha+127)/255;
}
/* PPU words and colors are read only; the output is an isolated presentation copy. */
void sm_decor_pixel(int wx,int wy,int screenx,int screeny,int layer,int brightness,const uint16_t *vram,const uint16_t *palette,int tile_base,uint8_t *out){
 int index=active_plane(wy);
 if((layer==1||layer==5)&&index>=0&&current->planes[index].enabled){
  const struct DecorPlane *p=&current->planes[index];
  int x=screenx+(int)round((double)layer1_x_pos*p->sx/100)-p->ox;
  int y=screeny+(int)round((double)layer1_y_pos*p->sy/100)-p->oy;
  blend(out,image_pixel(&images[index],x,y,p->repeat),255,brightness);
 }
 int bx=wx>=0?wx/16:-((-wx+15)/16),by=wy>=0?wy/16:-((-wy+15)/16);
 for(int l=1;l>=0;l--){
  const struct DecorCell *c=find_cell(l,bx,by);if(!c||(!c->custom&&!c->secret))continue;
  if(!c->secret&&((l==1&&layer!=1&&layer!=5)||(l==0&&(layer==4||layer==6))))continue;
  int px=wx&15,py=wy&15;if(c->tile&1024)px^=15;if(c->tile&2048)py^=15;
  uint8_t rgba[4];const uint8_t *color=NULL;
  if(c->custom){int cols=images[4].w/16;if(!cols)continue;int t=c->tile&1023;color=image_pixel(&images[4],(t%cols)*16+px,(t/cols)*16+py,0);}
  else{
   uint16_t entry=GET_WORD(g_ram+0xa000+(c->tile&1023)*8+2*((py/8)*2+px/8));
   int tx=px&7,ty=py&7;if(entry&0x4000)tx^=7;if(entry&0x8000)ty^=7;
   int address=(tile_base+(entry&1023)*16+ty)&32767;
   uint16_t a=vram[address],b=vram[(address+8)&32767];int bit=7-tx;
   int ci=((a>>bit)&1)|((a>>(bit+8)&1)<<1)|((b>>bit&1)<<2)|((b>>(bit+8)&1)<<3);
   if(!ci)continue;uint16_t rgb=palette[((entry>>10)&7)*16+ci];
   rgba[0]=((rgb>>10)&31)*255/31;rgba[1]=((rgb>>5)&31)*255/31;rgba[2]=(rgb&31)*255/31;rgba[3]=255;color=rgba;
  }
  int opacity=255;
  if(c->secret){
   float dx=fmaxf(fmaxf(bx*16-(int)samus_x_pos,0),(int)samus_x_pos-(bx+1)*16);
   float dy=fmaxf(fmaxf(by*16-(int)samus_y_pos,0),(int)samus_y_pos-(by+1)*16);
   float t=fminf(1,sqrtf(dx*dx+dy*dy)/c->radius);opacity=(int)(255*(c->opacity/100.f+(1-c->opacity/100.f)*t*t*(3-2*t)));
  }
  blend(out,color,opacity,brightness);
 }
}
