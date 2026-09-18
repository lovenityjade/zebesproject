#include "sm_cinematics.h"
#include "sm_locale.h"
#include "sm_credits.h"
#include "sm_bridge.h"
#include "sm_scene.h"
#include "sm_rtl.h"
#include "sm_cpu_infra.h"
#include "ida_types.h"
#include "enemy_types.h"
#include "variables.h"
#include "funcs.h"
#include <math.h>
#include <string.h>
#define N 32
static float lights[N*3*4];
static int scene,count,definitions[16];
static uint8_t visited[256*224];
static int queue[256*224];
const float *sm_cinema_lights(void){return lights;}
int sm_cinema_state(int f){
  if(f==0)return game_state==1?10:(game_state==2||game_state==4)?11:scene;
  if(f==1)return count;if(f==2)return cinematic_function;
  if(f==3)return game_state==4?menu_index:game_options_screen_index;
  if(f==4)return game_state==4?selected_save_slot:menu_option_index;
  return 0;
}
void sm_cinema_begin(int s){scene=s;count=0;memset(lights,0,sizeof(lights));}
void sm_cinema_sprite(int slot,int definition){if(slot>=0&&slot<16)definitions[slot]=definition;}
int sm_cinema_sprite_gui(int slot){
  if(slot<0||slot>=16)return 0;
  int d=definitions[slot];return d==0xa113||d==0xa125||d==0xce97||d==0xce9d||d==0xceaf||d==0xceb5||d==0xeec7||d==0xeecd||d==0xeefd||
    d==0xef03||d==0xef09||d==0xef0f||d==0xef15||d==0xef1b;
}
int sm_cinema_localized_sprite(int slot){
 if(slot<0 || slot>=16 || !sm_locale_get())return 0;
 int d=definitions[slot];
 const char* text;unsigned full_address;int dx=0,dy=0;
 switch(d){
 case 0xce97:text="COLONIE SPATIALE";full_address=0x8c921f;break;
 case 0xceaf:text="PLANÈTE ZEBES";full_address=0x8c9654;break;
 case 0xeec7:text="L'OPÉRATION EST";full_address=0x8caad3;break;
 case 0xeecd:text="TERMINÉE AVEC SUCCÈS";full_address=0x8cb3c7;dy=24;break;
 case 0xeefd:text="TEMPS ÉCOULÉ";full_address=0x8cb613;dx=-32;break;
 default:return 0;
 }
 unsigned address=0x8c0000u|cinematicspr_whattodraw[slot];
 const uint8_t* map=RomPtr(address);
 int pieces=GET_WORD(map),full=GET_WORD(RomPtr(full_address));
 if(!pieces || !full)return 0;
 int palette=128+((cinematicbg_arr9[slot]>>9)&7)*16;
 int color=0,value=-1;
 for(int i=1;i<16;i++){int c=g_snes->ppu->cgram[palette+i],v=(c&31)+((c>>5)&31)+((c>>10)&31);if(v>value){value=v;color=c;}}
 // The success lettering uses the original green accent, independent of
 // the grayscale planet background palette transition.
 if(d==0xeec7 || d==0xeecd)color=0x1fe0;
 sm_locale_caption(text,
   (int16_t)(cinematicbg_arr7[slot]-layer1_x_pos)+dx,
   (int16_t)(cinematicbg_arr8[slot]-layer1_y_pos)+dy,pieces,full,color);
 return 1;
}
static void add(int type,float x,float y,float radius,float r,float g,float b,float power,float dx,float dy,float length,float phase){
  if(count>=N||!isfinite(x)||!isfinite(y)||radius<=0||x < -100||x>356||y < -100||y>324)return;
  float *a=lights+count*4,*c=a+N*4,*d=c+N*4;
  a[0]=x;a[1]=y;a[2]=radius;a[3]=type;
  c[0]=r;c[1]=g;c[2]=b;c[3]=power*g_snes->ppu->brightness/15.f;
  d[0]=dx;d[1]=dy;d[2]=length;d[3]=phase;++count;
}
/* Inverse of the original Mode 7 affine mapping, including 13-bit scroll
 * clipping and screen flips. This never writes to the PPU or its registers. */
static int project(float u,float v,float *x,float *y,float *scale){
  const Ppu *p=g_snes->ppu;const int16_t *m=p->m7matrix;
  float det=(float)m[0]*m[3]-(float)m[1]*m[2];if(fabsf(det)<64)return 0;
  int cx=(int16_t)(m[4]<<3)>>3,cy=(int16_t)(m[5]<<3)>>3;
  int h=((int16_t)(m[6]<<3)>>3)-cx,k=((int16_t)(m[7]<<3)>>3)-cy;
  h=(h&0x2000)?h|~1023:h&1023;k=(k&0x2000)?k|~1023:k&1023;
  *x=((u-cx)*m[3]-(v-cy)*m[1])*256/det-h;
  *y=((v-cy)*m[0]-(u-cx)*m[2])*256/det-k;
  if(p->m7xFlip)*x=255-*x;if(p->m7yFlip)*y=255-*y;
  *scale=256/sqrtf(fabsf(det));return 1;
}
static void jet(float u,float v,float radius,float strength){
  float x,y,s,ex,ey,es;if(!project(u,v,&x,&y,&s)||!project(u,v+1,&ex,&ey,&es))return;
  if(x < -16||x>272||y < -16||y>240)return;
  float length=hypotf(ex-x,ey-y);if(length<.001f)return;
  // Warm outer exhaust, white-blue core: original ship silhouette stays intact.
  add(3,x,y,fminf(radius*s,14),1,.42f,.08f,strength,(ex-x)/length,(ey-y)/length,fminf(44*s,110),u*.19f);
}
/* Reject original hull details when selecting stars, using the same transform
 * as the renderer (a screen-aligned bounding box would fail during rotation). */
static int on_hull(float x,float y){
  if(g_snes->ppu->mode!=7||!(scene==2||scene==3||scene==4||scene==7||scene==8||scene==9))return 0;
  float ox,oy,s,ux,uy,vs,vx,vy;
  if(!project(0,0,&ox,&oy,&s)||!project(1,0,&ux,&uy,&vs)||!project(0,1,&vx,&vy,&vs))return 0;
  ux-=ox;uy-=oy;vx-=ox;vy-=oy;x-=ox;y-=oy;
  float det=ux*vy-uy*vx;if(fabsf(det)<.000001f)return 0;
  float u=(x*vy-y*vx)/det,v=(y*ux-x*uy)/det;
  return u>=-2&&u<=114&&v>=-2&&v<=(scene==3?98:50);
}
static int bright(int pos){
  const uint8_t *p=sm_scene_pixels+pos*4,*ui=sm_ui_overlay()+pos*4;
  return !ui[3] && (p[0]>14||p[1]>14||p[2]>14);
}
static void stars(void){
  memset(visited,0,sizeof(visited));
  for(int seed=0;seed<256*224 && count<N;seed++){
    if(visited[seed]||!bright(seed))continue;
    int head=0,tail=1,minx=seed%256,maxx=minx,miny=seed/256,maxy=miny,best=seed,value=0;queue[0]=seed;visited[seed]=1;
    while(head<tail){
      int pos=queue[head++],x=pos%256,y=pos/256;
      if(x<minx)minx=x;if(x>maxx)maxx=x;if(y<miny)miny=y;if(y>maxy)maxy=y;
      const uint8_t *p=sm_scene_pixels+pos*4;int v=p[0]+p[1]+p[2];if(v>value){value=v;best=pos;}
      for(int dy=-1;dy<=1;dy++)for(int dx=-1;dx<=1;dx++){
        int nx=x+dx,ny=y+dy;if(nx<0||nx>=256||ny<0||ny>=224)continue;int n=ny*256+nx;
        if(!visited[n]&&bright(n)){visited[n]=1;queue[tail++]=n;}
      }
    }
    // Only isolated authored star sprites/pixels. Large objects and font strokes
    // are excluded, so station panels and hull highlights never become stars.
    if(tail<=20 && maxx-minx<=5 && maxy-miny<=5 && value>100){
      float x=(minx+maxx)*.5f,y=(miny+maxy)*.5f;int inside=on_hull(x,y);
      for(int i=0;i<count;i++)if(lights[i*4+3]==4){float dx=x-lights[i*4],dy=y-lights[i*4+1];inside|=dx*dx+dy*dy<lights[i*4+2]*lights[i*4+2];}
      if(!inside){const uint8_t *p=sm_scene_pixels+best*4;add(1,x,y,2,.45f+p[2]/510.f,.5f+p[1]/510.f,.6f+p[0]/638.f,.55f,0,0,0,x*.73f+y*.21f);}
    }
  }
}
static void beacons(void){
  // Actual red docking-ring lamps in 95:A82F / 96:FE69 + 0x600.
  // Sample their current rendered palette so a native fade never leaves a glow.
  static const float anchors[][2]={{43.5f,33.5f},{34.5f,43.5f},{48.5f,40.5f},{40,49}};
  for(int i=0;i<4;i++){
    float x,y,s;if(!project(anchors[i][0],anchors[i][1],&x,&y,&s))continue;
    float power=0;
    for(int dy=-2;dy<=2;dy++)for(int dx=-2;dx<=2;dx++){
      int px=(int)roundf(x)+dx,py=(int)roundf(y)+dy;if(px<0||px>=256||py<0||py>=224)continue;
      const uint8_t *p=sm_scene_pixels+(py*256+px)*4;
      if(p[2]>p[1]*2&&p[2]>p[0]*2)power=fmaxf(power,p[2]/255.f);
    }
    if(power>.1f)add(2,x,y,fmaxf(2,fminf(7,6*s)),1,.12f,.055f,.7f*power,0,0,0,i);
  }
}
/* The title laboratory is a single authored Mode 7 image. These anchors are
 * the two terminal screens, containment tank and blue machinery, in its own
 * texture coordinates. Sample the current palette for the opening color cycle. */
static void title_lamp(float u,float v,float radius,float strength){
  float x,y,s;if(!project(u,v,&x,&y,&s)||x<0||x>=256||y<0||y>=224)return;
  float r=0,g=0,b=0;int best=0;
  for(int dy=-3;dy<=3;dy++)for(int dx=-3;dx<=3;dx++){
    int px=(int)x+dx,py=(int)y+dy;if(px<0||px>=256||py<0||py>=224)continue;
    int n=py*256+px;const uint8_t *p=sm_scene_pixels+n*4;
    if(sm_ui_overlay()[n*4+3])continue;
    int value=p[0]+p[1]+p[2];
    if(value>best){best=value;r=p[2]/255.f;g=p[1]/255.f;b=p[0]/255.f;}
  }
  if(best>24)add(2,x,y,fminf(radius*s,40),r,g,b,strength,0,0,0,u*.03f);
}
static void title_laboratory(void){
  if(g_snes->ppu->mode!=7||!g_snes->ppu->layer[0].mainScreenEnabled)return;
  title_lamp(70,152,12,.65f);title_lamp(185,147,12,.65f);
  title_lamp(128,142,10,.48f);title_lamp(128,183,17,.28f);
  title_lamp(128,97,14,.22f);
  float x,y,s;
  if(project(128,205,&x,&y,&s)&&s<2.5f)
    add(5,x,y,104*s,.10f,.32f,.36f,.16f,0,0,12*s,0);
}
void sm_cinema_frame(void){
  count=0;memset(lights,0,sizeof(lights));
  if(!g_snes||g_snes->ppu->forcedBlank||sm_credits_state(0))return;
  const int kind=sm_presentation_kind();
  if(game_state==1){title_laboratory();return;}
  if(game_state==2 || game_state==4){
    const Ppu *p=g_snes->ppu;
    if(p->mode==1 && p->bgLayer[0].tilemapAdr==0x5000 && p->bgLayer[1].tilemapAdr==0x5800 && p->layer[1].mainScreenEnabled){
      add(4,123,99,49.5f,1,.55f,.14f,.55f,0,0,1.05f,0);stars();
    }
    return;
  }
  if((kind==1||game_state==38) && (game_state==8||game_state==38) && room_ptr==0x91f8){
    const Enemy_GunshipTop *e=Get_GunshipTop(0);int f=e->gtp_var_F;
    if(f==0xa7d8||f==0xa80c||f==0xa8d0||f==0xac1b||f==0xacd7||f==0xad0e||f==0xad2d){
      float power=f==0xa8d0?.8f:1.4f;
      for(int i=-1;i<=1;i+=2)add(3,(int)e->base.x_pos-layer1_x_pos+i*52,(int)e->base.y_pos-layer1_y_pos+50,8,1,.42f,.08f,power,0,1,55,i*2);
    }
    return;
  }
  if(kind!=2||!scene)return;
  if(scene==2 && g_snes->ppu->mode==7){jet(28,35,4,1.5f);jet(84,35,4,1.5f);}
  if(scene==4){
    if(g_snes->ppu->mode==1)add(4,123,99,49.5f,1,.55f,.14f,.55f,0,0,1.05f,0);
    else{
      for(int i=0;i<16;i++)if(definitions[i]==0xcea3 && cinematicspr_instr_ptr[i]){
        add(4,(int16_t)cinematicbg_arr7[i]-13,(int16_t)cinematicbg_arr8[i]-12,49.5f,1,.55f,.14f,.55f,0,0,1.05f,0);break;
      }
      if(g_snes->ppu->layer[0].mainScreenEnabled)jet(56,33,6,1.8f);
    }
  }
  if(scene==5 && g_snes->ppu->mode==7){
    float x,y,s;if(project(128,128,&x,&y,&s)&&s<3)add(4,x,y,86*s,.32f,.6f,1,.5f,0,0,1,0);
  }
  if(scene==6){
    if(g_snes->ppu->mode==7){
      float x,y,s;if(project(128,128,&x,&y,&s)&&s<3)
        add(4,x,y,86*s,1,.38f,.12f,.65f,0,0,1,0);
    }
    if(cinematic_function==FUNC16(CinematicFunction_Intro_Func117)){
      float t=fmaxf(0,fminf(1,1-cinematic_var4/63.f));
      add(7,128,112,12+145*t,1,.42f,.16f,.8f*(1-t),0,0,0,t);
    }
  }
  if(scene==7){
    if(g_snes->ppu->mode==7){jet(28,35,4,1.6f);jet(84,35,4,1.6f);}
    // EF21 is the original rescued-animals ship. Follow its actual position;
    // never invent a rescue or key this effect to a wall-clock timer.
    for(int i=0;i<16;i++)if(definitions[i]==0xef21 && cinematicspr_instr_ptr[i] && cinematicspr_whattodraw[i]){
      float x=(int16_t)(cinematicbg_arr7[i]-layer1_x_pos);
      float y=(int16_t)(cinematicbg_arr8[i]-layer1_y_pos);
      add(6,x,y,4.5f,.12f,.6f,1,1.25f,1,0,26,i);
    }
  }
  if(scene==8 && g_snes->ppu->mode==7)jet(56,33,6,1.8f);
  if(scene==9 && g_snes->ppu->mode==7){jet(28,35,4,1.8f);jet(84,35,4,1.8f);}
  if(scene==3 && g_snes->ppu->mode==7)beacons();
  if((scene>=2 && scene<=5)||scene==7||scene==8||scene==9)stars();
}
