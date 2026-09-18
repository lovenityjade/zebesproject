#include "sm_locale.h"
#include "sm_seed_atlas.h"
#include "sm_seed.h"
#include "sm_areas.h"
#include "sm_connections.h"
#include "sm_minimizer.h"
#include "sm_map_browser.h"
#include "sm_escape.h"
#include "ida_types.h"
#include "variables.h"
#include "funcs.h"
#include "sm_rtl.h"
#include "sm_cpu_infra.h"
#include <string.h>
#include <stdlib.h>
#include "sm_seed_atlas.inc"

extern uint8_t sm_wide_hud[];
typedef struct {int minx,miny,maxx,maxy,x,y,visible;} Piece;
static Piece pieces[ATLAS_NODES];
static int16_t tile_nodes[6][32][64];
static int routes[40],ready,cell=8,camera_x,camera_y,max_x,max_y,was_active;
static uint8_t adjacency[ATLAS_NODES][ATLAS_NODES];
static int route(int i){
  if(sm_minimizer_active())return sm_minimizer_destination(i);
  int r=i<32?sm_areas_destination(i):sm_connections_destination(i-32);
  return r<0?atlas_portals[i].vanilla:r+(i>=32?32:0);
}
int sm_seed_atlas_available(void){
  return sm_seed_active() && (sm_minimizer_active() || sm_areas_destination(0)>=0 || sm_connections_destination(0)>=0);
}
int sm_seed_atlas_active(void){
  return sm_seed_atlas_available() && (game_state==14 || game_state==15 || game_state==16) &&
    (menu_index==0 || menu_index==2 || menu_index==7) && !pause_screen_mode && sm_map_browser_state(0)==0;
}
void sm_seed_atlas_reset(void){ready=was_active=0;cell=8;}
int sm_seed_atlas_cell_size(void){return cell;}
static void link(int a,int b){if(a>=0 && b>=0 && a<ATLAS_NODES && b<ATLAS_NODES && pieces[a].visible && pieces[b].visible)adjacency[a][b]=adjacency[b][a]=1;}
static int node_at(int area,int x,int y){return area>=0 && area<6 && x>=0 && x<64 && y>=0 && y<32?tile_nodes[area][y][x]:-1;}
static int tile_present(int a,int x,int y,int region){
  int byte=((x&31)>>3)+4*((x&32)+y),mask=0x80>>(x&7);
  return (sm_seed_map_data(a)[byte]&mask) && sm_minimizer_map_tile(region,a,byte,mask) && sm_escape_map_tile(a,byte,mask);
}
static void build(void){
  memset(tile_nodes,0xff,sizeof(tile_nodes));memset(pieces,0,sizeof(pieces));memset(adjacency,0,sizeof(adjacency));
  for(int n=0;n<ATLAS_NODES;n++){pieces[n].minx=64;pieces[n].miny=32;}
  for(unsigned i=0;i<sizeof(atlas_tiles)/sizeof(*atlas_tiles);i++){
    int a=atlas_tiles[i].area,x=atlas_tiles[i].x,y=atlas_tiles[i].y,n=atlas_tiles[i].node;
    if(!tile_present(a,x,y,atlas_tiles[i].region))continue;
    tile_nodes[a][y][x]=n;Piece *p=&pieces[n];p->visible=1;
    if(x<p->minx)p->minx=x;if(y<p->miny)p->miny=y;
    if(x>p->maxx)p->maxx=x;if(y>p->maxy)p->maxy=y;
  }
  for(unsigned i=0;i<sizeof(atlas_fixed)/sizeof(*atlas_fixed);i++)link(atlas_fixed[i][0],atlas_fixed[i][1]);
  for(int i=0;i<40;i++){
    routes[i]=route(i);
    if(routes[i]>=0 && routes[i]<40 && routes[i]!=i)link(atlas_portals[i].node,atlas_portals[routes[i]].node);
  }
  /* Stable breadth-first packing from Landing Site. Changing connections
   * changes placement, never the authored shape/orientation of a room. */
  int order[ATLAS_NODES],used[ATLAS_NODES]={0},count=0;
  for(int root=0;root<ATLAS_NODES;root++){
    if(used[root] || !pieces[root].visible)continue;
    int head=count;used[root]=1;order[count++]=root;
    while(head<count){int n=order[head++];for(int m=0;m<ATLAS_NODES;m++)if(adjacency[n][m]&&!used[m]){used[m]=1;order[count++]=m;}}
  }
  int colwidth[3]={0},rowheight[ATLAS_NODES]={0};
  for(int i=0;i<count;i++){
    Piece *p=&pieces[order[i]];int w=p->maxx-p->minx+9,h=p->maxy-p->miny+9;
    if(w<24)w=24;
    if(w>colwidth[i%3])colwidth[i%3]=w;if(h>rowheight[i/3])rowheight[i/3]=h;
  }
  max_x=max_y=0;
  for(int i=0;i<count;i++){
    Piece *p=&pieces[order[i]];p->x=4;p->y=4;
    for(int c=0;c<i%3;c++)p->x+=colwidth[c];
    for(int r=0;r<i/3;r++)p->y+=rowheight[r];
    if(p->x+p->maxx-p->minx+4>max_x)max_x=p->x+p->maxx-p->minx+4;
    if(p->y+p->maxy-p->miny+4>max_y)max_y=p->y+p->maxy-p->miny+4;
  }
  ready=1;
}
static void ensure(void){
  if(ready)for(int i=0;i<40;i++)if(route(i)!=routes[i]){ready=was_active=0;break;}
  if(!ready)build();
}
int sm_seed_atlas_location(int area,int x,int y,int *out_x,int *out_y){
  if(!out_x || !out_y || !sm_seed_atlas_available())return 0;
  ensure();int n=node_at(area,x,y);if(n<0)return 0;
  *out_x=pieces[n].x+x-pieces[n].minx;*out_y=pieces[n].y+y-pieces[n].miny;return 1;
}
int sm_seed_atlas_focus(int area,int x,int y){
  int wx,wy;if(!sm_seed_atlas_location(area,x,y,&wx,&wy))return 0;
  camera_x=wx*8+4;camera_y=wy*8+4;was_active=1;return 1;
}
int sm_seed_atlas_project(int area,int mx,int my,int width,int *x,int *y){
  int wx,wy;if(mx<0 || my<0 || !sm_seed_atlas_location(area,mx/8,my/8,&wx,&wy))return 0;
  *x=width/2+(wx*8+mx%8)*cell/8-camera_x*cell/8;
  *y=120+(wy*8+my%8)*cell/8-camera_y*cell/8;return 1;
}
static void focus_samus(void){
  sm_seed_atlas_focus(area_index,room_x_coordinate_on_map+(samus_x_pos>>8),room_y_coordinate_on_map+(samus_y_pos>>8)+1);
}
void sm_seed_atlas_focus_area(int area){
  ensure();
  if(area==area_index){focus_samus();return;}
  int counts[ATLAS_NODES]={0},best=-1;
  for(int y=0;y<32;y++)for(int x=0;x<64;x++){int n=node_at(area,x,y);if(n>=0)counts[n]++;}
  for(int n=0;n<ATLAS_NODES;n++)if(counts[n] && (best<0 || counts[n]>counts[best]))best=n;
  if(best<0)return;
  Piece *p=&pieces[best];int distance=10000,bx=0,by=0;
  for(int y=0;y<32;y++)for(int x=0;x<64;x++)if(node_at(area,x,y)==best){
    int d=abs(2*x-p->minx-p->maxx)+abs(2*y-p->miny-p->maxy);
    if(d<distance){distance=d;bx=x;by=y;}
  }
  sm_seed_atlas_focus(area,bx,by);
}
int sm_seed_atlas_tick(void){
  if(!sm_seed_atlas_active() || game_state!=15 || menu_index)return 0;
  ensure();if(!was_active)focus_samus();
  unsigned keys=joypad1_newkeys,held=joypad1_lastkeys;
  if(keys&kButton_Y){cell=cell==8?4:cell==4?2:8;QueueSfx1_Max6(0x37);}
  if(keys&kButton_A)focus_samus();
  int speed=16/cell;
  camera_x+=((!!(held&kButton_Right))-(!!(held&kButton_Left)))*speed;
  camera_y+=((!!(held&kButton_Down))-(!!(held&kButton_Up)))*speed;
  if(camera_x<0)camera_x=0;if(camera_x>max_x*8)camera_x=max_x*8;
  if(camera_y<0)camera_y=0;if(camera_y>max_y*8)camera_y=max_y*8;
  HandlePauseScreenLR();HandlePauseScreenStart();HandleHudTilemap();HandlePauseScreenPaletteAnimation();
  return 1;
}
static void put(uint8_t *out,int width,int x,int y,uint32_t c){
  if(x<8||x>=width-8||y<48||y>=192)return;
  int b=g_snes->ppu->forcedBlank?0:g_snes->ppu->brightness;
  uint8_t *p=out+(y*width+x)*4;p[0]=(c&255)*b/15;p[1]=((c>>8)&255)*b/15;p[2]=(c>>16)*b/15;p[3]=255;
}
static void line(uint8_t *out,int width,int x0,int y0,int x1,int y1){
  int dx=abs(x1-x0),sx=x0<x1?1:-1,dy=-abs(y1-y0),sy=y0<y1?1:-1,err=dx+dy,step=0;
  for(;;){if((step++&3)<2)put(out,width,x0,y0,0x507c88);if(x0==x1&&y0==y1)break;int e=err*2;if(e>=dy){err+=dy;x0+=sx;}if(e<=dx){err+=dx;y0+=sy;}}
}
/* Decode the original menu spritemap and currently uploaded OBJ graphics.
 * This keeps stations, the ship and Samus's blinking cursor ROM-authored. */
static void sprite(uint8_t *out,int width,int area,int mx,int my,int id,int palette){
  int x,y;if(!sm_seed_atlas_project(area,mx,my,width,&x,&y))return;
  const uint8_t *p=RomPtr_82(GET_WORD(RomFixedPtr(0x82c569)+2*id));int count=GET_WORD(p);p+=2;
  Ppu *ppu=g_snes->ppu;
  for(int i=0;i<count;i++,p+=5){
    int off=GET_WORD(p),ox=(off&255)-((off&256)?256:0),oy=(int8_t)p[2];
    int size=(off&0x8000)?16:8,a=(GET_WORD(p+3)&0xf1ff)|palette;
    for(int py=0;py<size*cell/8;py++)for(int px=0;px<size*cell/8;px++){
      int tx=px*8/cell,ty=py*8/cell;if(a&0x4000)tx=size-1-tx;if(a&0x8000)ty=size-1-ty;
      int tile=(((a&255)/16+ty/8)*16)|(((a&15)+tx/8)&15);
      int addr=(((a&256)?ppu->objTileAdr2:ppu->objTileAdr1)+tile*16)&32767;
      int col=7-(tx&7),row=ty&7,w=ppu->vram[(addr+row)&32767],w2=ppu->vram[(addr+row+8)&32767];
      int index=((w>>col)&1)|(((w>>(8+col))&1)<<1)|(((w2>>col)&1)<<2)|(((w2>>(8+col))&1)<<3);
      if(!index)continue;uint16_t c=ppu->cgram[128+((a>>9)&7)*16+index];
      put(out,width,x+ox*cell/8+px,y+oy*cell/8+py-1,((c&31)*255/31)<<16|(((c>>5)&31)*255/31)<<8|((c>>10)&31)*255/31);
    }
  }
}
static void stations(uint8_t *out,int width){
  const int lists[]={0xc7db,0xc7eb,0xc7fb,0xc80b},ids[]={11,10,0x4e,8};
  for(int a=0;a<6;a++)for(int type=0;type<4;type++){
    unsigned table=GET_WORD(RomPtr_82(lists[type]+a*2));if(table<0x8000)continue;
    const uint8_t *xy=RomPtr_82(table);
    for(int n=0;n<16;n++){
      int x=GET_WORD(xy+4*n),y=GET_WORD(xy+4*n+2);if(x==65535)break;if(x>=65534||y>=65534)continue;
      if(type==3 && (n>=8 || !(used_save_stations_and_elevators[a*2]&(1<<n))))continue;
      sprite(out,width,a,x,y,ids[type],type==3?1024:3584);
    }
  }
  sprite(out,width,0,0xd8,0x28,0x63,3584);
}
static void draw(uint8_t *out,int width){
  for(int y=48;y<192;y++)for(int x=8;x<width-8;x++)put(out,width,x,y,(x%cell==0&&y%cell==0)?0x181028:0x05040c);
  for(int i=0;i<40;i++){
    int j=routes[i],x,y,tx,ty;if(j<=i || j>=40)continue;
    if(!sm_seed_atlas_project(atlas_portals[i].area,atlas_portals[i].x*8+4,atlas_portals[i].y*8+4,width,&x,&y) ||
       !sm_seed_atlas_project(atlas_portals[j].area,atlas_portals[j].x*8+4,atlas_portals[j].y*8+4,width,&tx,&ty))continue;
    line(out,width,x,y,tx,y);line(out,width,tx,y,tx,ty);
  }
  for(unsigned i=0;i<sizeof(atlas_fixed)/sizeof(*atlas_fixed);i++){
    Piece *a=&pieces[atlas_fixed[i][0]],*b=&pieces[atlas_fixed[i][1]];if(!a->visible||!b->visible)continue;
    int x=width/2+(a->x+(a->maxx-a->minx)/2)*cell-camera_x*cell/8,y=120+(a->y+(a->maxy-a->miny)/2)*cell-camera_y*cell/8;
    int tx=width/2+(b->x+(b->maxx-b->minx)/2)*cell-camera_x*cell/8,ty=120+(b->y+(b->maxy-b->miny)/2)*cell-camera_y*cell/8;
    line(out,width,x,y,tx,y);line(out,width,tx,y,tx,ty);
  }
  const uint8_t *gfx=RomFixedPtr(0xb68000);const uint16_t *pal=(const uint16_t*)RomFixedPtr(0xb6f000);
  for(unsigned i=0;i<sizeof(atlas_tiles)/sizeof(*atlas_tiles);i++){
    int a=atlas_tiles[i].area,mx=atlas_tiles[i].x,my=atlas_tiles[i].y,x,y;
    if(!sm_seed_atlas_project(a,mx*8,my*8,width,&x,&y) || x+cell<=8 || x>=width-8 || y+cell<=48 || y>=192)continue;
    const uint8_t *ptr=RomFixedPtr(0x82964a)+a*3;
    const uint16_t *map=(const uint16_t*)RomPtr(ptr[0]|ptr[1]<<8|ptr[2]<<16);
    uint16_t tile=map[(mx&31)+my*32+(mx&32)*32];
    int byte=((mx&31)>>3)+4*((mx&32)+my),mask=0x80>>(mx&7);
    const uint8_t *explored=a==area_index?map_tiles_explored:(uint8_t*)explored_map_tiles_saved+a*256;
    if(explored[byte]&mask)tile&=~0x400;
    const uint8_t *t=gfx+(tile&1023)*32;
    for(int py=0;py<cell;py++)for(int px=0;px<cell;px++){
      int row=py*8/cell,col=px*8/cell,index=0;row=tile&0x8000?7-row:row;col=tile&0x4000?col:7-col;
      for(int p=0;p<4;p++)index|=((t[row*2+p/2*16+p%2]>>col)&1)<<p;
      uint16_t c=pal[((tile>>10)&7)*16+index];
      put(out,width,x+px,y+py,((c&31)*255/31)<<16|(((c>>5)&31)*255/31)<<8|((c>>10)&31)*255/31);
    }
  }
  stations(out,width);
  /* The old area heading must not name a different area under the camera. */
  for(int y=32;y<48;y++)for(int x=width/2-56;x<width/2+56;x++){
    uint8_t *p=out+(y*width+x)*4;memset(p,0,3);p[3]=255;
  }
  sm_native_map_text(out,width,width/2-sm_locale_length(sm_locale_text("SEED MAP"))*4,40,"SEED MAP",0xffffff);
  /* Labels only where a complete native glyph line fits inside the viewport. */
  for(int n=0;n<ATLAS_NODES;n++)if(pieces[n].visible && cell==8){
    int x=width/2+(pieces[n].x*8-camera_x)*cell/8,y=120+((pieces[n].y-2)*8-camera_y)*cell/8;
    if(x>=8 && x+sm_locale_length(sm_locale_text(atlas_names[n]))*8<width-8 && y>=48 && y+9<192)
      sm_native_map_text(out,width,x,y,atlas_names[n],0x91b5c8);
  }
}
void sm_seed_atlas_render(uint8_t *pixels){
  if(!sm_seed_atlas_active()){was_active=0;return;}ensure();if(!was_active)focus_samus();
  draw(pixels,256);draw((uint8_t*)sm_ui_overlay(),256);draw(sm_wide_hud,400);
}
static void cursor(uint8_t *out,int width){
  static const int frames[]={0x5f,0x60,0x61,0x60};
  sprite(out,width,area_index,(room_x_coordinate_on_map+(samus_x_pos>>8))*8,
      (room_y_coordinate_on_map+(samus_y_pos>>8)+1)*8,frames[(frame_counter_every_frame/8)%4],3584);
}
void sm_seed_atlas_cursor(uint8_t *pixels){
  if(!sm_seed_atlas_active())return;cursor(pixels,256);cursor((uint8_t*)sm_ui_overlay(),256);cursor(sm_wide_hud,400);
}
/* The minimap remains a local room view. Hide old adjacent tiles belonging
 * to another shuffled piece: proximity in the ROM is no longer a connection. */
int sm_seed_atlas_local(int area,int x,int y){
  if(!sm_seed_atlas_available())return 1;ensure();
  int current=node_at(area_index,room_x_coordinate_on_map+(samus_x_pos>>8),room_y_coordinate_on_map+(samus_y_pos>>8)+1);
  return current>=0 && node_at(area,x,y)==current;
}
static void minimap(uint8_t *out,int width){
  int mx=room_x_coordinate_on_map+(samus_x_pos>>8),my=room_y_coordinate_on_map+(samus_y_pos>>8)+1;
  int left=width==400?336:208,cols=width==400?7:5,rows=width==400?4:3,center=width==400?4:2;
  for(int y=0;y<rows;y++)for(int x=0;x<cols;x++)if(!sm_seed_atlas_local(area_index,mx+x-center,my+y-1))
    for(int py=0;py<8;py++)for(int px=0;px<8;px++){
      uint8_t *p=out+((y*8+py)*width+left+x*8+px)*4;memset(p,0,3);p[3]=255;
    }
}
void sm_seed_atlas_minimap(uint8_t *pixels){
  if(!sm_seed_atlas_available() || game_state!=8)return;
  minimap(pixels,256);minimap((uint8_t*)sm_ui_overlay(),256);minimap(sm_wide_hud,400);
}
