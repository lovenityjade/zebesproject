#include "sm_credits.h"
#include "sm_map_browser.h"
#include "sm_objective_pause.h"
/* Keep the upstream PPU intact and extend only its public scanline entry.
 * This exposes layer provenance, BG2 offsets and an authored Ceres backdrop.
 * All altered PPU fields are restored before game code resumes. */
#define ppu_runLine sm_reference_runLine
#include "sm_ppu_decoder.c"
#undef ppu_runLine
#include "sm_scene.h"
#include "sm_bridge.h"
#include "sm_seed.h"
#include "sm_effects.h"
#include "sm_profile.h"
#include "variables.h"
#include "sm_rtl.h"
#include "sm_cpu_infra.h"
#include "sm_emitters.generated.h"
#include "sm_background.generated.h"
#include <math.h>
uint8_t sm_scene_background_mask[256*240];
uint8_t sm_scene_emission[256*240*4], sm_scene_lights[64*60*4];
static uint32_t hashes[2048];
static uint8_t hash_valid[2048];
static uint32_t tile_indices[2048][8];
static uint64_t background_masks[2048];
static uint32_t tile_hash(Ppu *p,int address,int mode) {
  int key=mode==7?address:address/16; key &= 2047;
  if(hash_valid[key]) return hashes[key];
  uint32_t h=2166136261u;
  for(int j=0;j<(mode==7?64:32);++j) {
    uint8_t b=mode==7?p->vram[address*64+j]>>8:((uint8_t*)p->vram)[(address*2+j)&65535];
    h=(h^b)*16777619u;
  }
  memset(tile_indices[key],0,sizeof(tile_indices[key]));
  for(unsigned i=0;i<sizeof(emitter_rules)/sizeof(*emitter_rules);++i)
    if(emitter_rules[i].hash==h && emitter_rules[i].mode==mode)
      for(int w=0;w<8;++w)tile_indices[key][w]|=emitter_rules[i].indices[w];
  background_masks[key]=0;
  if(mode==7)for(unsigned i=0;i<sizeof(background_rules)/sizeof(*background_rules);++i)
    if(background_rules[i].hash==h)background_masks[key]|=background_rules[i].mask;
  hash_valid[key]=1;return hashes[key]=h;
}
static int tile_emits(Ppu *p,int address,int mode,int index) {
  tile_hash(p,address,mode);
  int key=(mode==7?address:address/16)&2047;
  return (tile_indices[key][index/32]>>(index%32))&1;
}
static void mark(Ppu *p,int x,int y,int index) {
  uint16_t c=p->cgram[index&255]; uint8_t *d=sm_scene_emission+(y*256+x)*4;
  d[0]=((c>>10)&31)*255/31;d[1]=((c>>5)&31)*255/31;d[2]=(c&31)*255/31;
}


uint8_t sm_scene_pixels[256*240*4];
uint8_t sm_scene_layers[256*240*4];
static uint8_t sm_ui_pixels[256*240*4];
uint16_t sm_hud_palette[32];
const uint8_t *sm_ui_overlay(void) {return sm_ui_pixels;}
int sm_scene_active, sm_scene_parallax;
int sm_scene_weather;
int sm_scene_camera_x, sm_scene_camera_y;
static uint8_t stable_hud[256*32*4],stable_wide_hud[400*32*4];
static int stable_hud_brightness;
void sm_scene_reset(void) {
  stable_hud_brightness=0;
  memset(sm_scene_pixels,0,sizeof(sm_scene_pixels));
  memset(sm_scene_layers,0,sizeof(sm_scene_layers));
}

// Batch decoder from the pinned core, restricted to the modes it implements.
// The original reference scanline still runs unchanged for native pixels.
static int sm_fast_scene=1;
void sm_set_fast_scene_render(int enabled){sm_fast_scene=!!enabled;}
static int sm_fast_line_supported(Ppu *p) {
  if(!sm_fast_scene || p->mode!=1 || p->pseudoHires || p->interlace)return 0;
  if(!p->bg3priority && (p->layer[2].mainScreenEnabled || p->layer[2].subScreenEnabled))return 0;
  for(int i=0;i<3;i++)if(p->bgLayer[i].bigTiles || (p->mosaicSize>1 && p->bgLayer[i].mosaicEnabled))return 0;
  return 1;
}
static void sm_draw_full_line(Ppu *p,int line) {
  if(!sm_fast_line_supported(p)) {for(int x=0;x<256;x++)ppu_handlePixel(p,x,line);return;}
  uint8_t screens[2]={p->screenEnabled[0],p->screenEnabled[1]};
  uint8_t windows[2]={p->screenWindowed[0],p->screenWindowed[1]};
  p->screenEnabled[0]=p->screenEnabled[1]=p->screenWindowed[0]=p->screenWindowed[1]=0;
  for(int i=0;i<5;i++) {
    p->screenEnabled[0]|=p->layer[i].mainScreenEnabled<<i;p->screenEnabled[1]|=p->layer[i].subScreenEnabled<<i;
    p->screenWindowed[0]|=p->layer[i].mainScreenWindowed<<i;p->screenWindowed[1]|=p->layer[i].subScreenWindowed<<i;
  }
  PpuDrawWholeLine(p,line);
  for(int x=0;x<256;x++)sm_last_main_layer[x]=(p->bgBuffers[0].data[x]>>8)&15;
  memcpy(p->screenEnabled,screens,2);memcpy(p->screenWindowed,windows,2);
}

static int sm_scene_finite_bg2(const Ppu *p) {
  // Mother Brain switches BG2 to a 256px body tilemap for her ascent and
  // phases 2/3 (A9:8D11). Phase 1 and ordinary tiled backgrounds stay native.
  return p->mode==1 && room_ptr==0xdd58 && layer2_scroll_x==1 &&
      layer2_scroll_y==1 && !p->bgLayer[1].tilemapWider;
}

/* Only the presentation PPU copy loses the 256px suit beam. Native HDMA,
 * acquisition, palette transition, sprite animation and reference pixels run
 * unchanged. Both decoders have their own window representation. */
static void sm_scene_replace_suit(Ppu *p) {
  memset(p->mathEnabled,0,sizeof(p->mathEnabled));
  memset(p->windowLayer,0,sizeof(p->windowLayer));p->windowsel=0;
  p->clipMode=p->preventMathMode=0;
  for(int i=0;i<5;i++)p->layer[i].mainScreenWindowed=p->layer[i].subScreenWindowed=false;
}
#include "sm_wide.inc"

static Ppu combat_ppu;
static void sm_wide_profiled(Ppu *p,int line) {
  uint64_t t=sm_clock_ns();sm_wide_line(p,line);
  if(sm_decor_room_active() && sm_scene_active && p->mode==1 && !p->forcedBlank && line>32 && line<=224) {
    memcpy(sm_scene_pixels+(line-1)*1024,sm_wide_pixels+((line-1)*400+72)*4,1024);
    memcpy(sm_scene_layers+(line-1)*1024,sm_wide_layers+((line-1)*400+72)*4,1024);
  }
  sm_profile_ns[3]+=sm_clock_ns()-t;
}
void ppu_runLine(Ppu *ppu, int line) {
  if(line==1) { memcpy(sm_hud_palette,ppu->cgram,sizeof(sm_hud_palette));sm_scene_active=sm_presentation_kind()==1;memset(sm_ui_pixels,0,sizeof(sm_ui_pixels));memset(sm_scene_background_mask,0,sizeof(sm_scene_background_mask)); memset(hash_valid,0,sizeof(hash_valid)); memset(sm_scene_emission,0,sizeof(sm_scene_emission)); }
  uint64_t profile_start=sm_clock_ns();
  sm_reference_runLine(ppu,line);
  sm_profile_ns[1]+=sm_clock_ns()-profile_start;
  profile_start=sm_clock_ns();
  if (line < 1 || line > 224) return;
  const int replace_suit=sm_combat_effects && sm_visual_suit();
  const int replace_pb=replace_suit || (sm_combat_effects && game_state==8 && ((power_bomb_explosion_status&0x8000) || sm_visual_eye()));
  if(replace_pb && line>32) {
    memcpy(&combat_ppu,ppu,sizeof(combat_ppu));ppu=&combat_ppu;
    memset(ppu->mathEnabled,0,sizeof(ppu->mathEnabled));ppu->clipMode=0;
    if(replace_suit)sm_scene_replace_suit(ppu);
  }
  uint8_t *dst = sm_scene_pixels+(line-1)*256*4;
  uint8_t *meta = sm_scene_layers+(line-1)*256*4;
  memcpy(dst, ppu->renderBuffer+(line-1)*ppu->renderPitch,256*4);
  memset(meta,0,256*4);
  // Message text is BG3. Save its exact visible pixels before removing it
  // from the atmosphere input; the UI is composed after the shader.
  const int frontend=(game_state==2 || game_state==4) && ppu->mode==1 && ppu->bgLayer[0].tilemapAdr==0x5000;
  const int message=sm_scene_active && sm_message_active() && line>32;
  // SetupPpu_Intro assigns the story's text tilemap to BG3 at word 0x4c00.
  const int story_text=sm_presentation_kind()==2 && ppu->mode==1 && ppu->bgLayer[2].tilemapAdr==0x4c00;
  if((message || story_text || frontend) && !ppu->forcedBlank) {
    for(int x=0;x<256;x++)if(frontend ? (sm_last_main_layer[x]==0 || sm_last_main_layer[x]==4 || sm_last_main_layer[x]==6) : sm_last_main_layer[x]==2) {
      uint8_t *ui=sm_ui_pixels+((line-1)*256+x)*4;
      memcpy(ui,dst+x*4,4);ui[3]=255;
    }
  }
  int clipped=0;
  if(sm_scene_active && !ppu->forcedBlank) {
    for(int i=0;i<256;i+=2) {
      int y;uint32_t words=(uint32_t)ppu->oam[i]|(uint32_t)ppu->oam[i+1]<<16;
      if(!sm_sprite_view_y(i,words,&y))continue;
      int height=spriteSizes[ppu->objSize][(ppu->highOam[i>>3]>>((i&7)+1))&1];
      if(line-1>=y && line-1<y+height)continue;
      /* Only rebuild the scanline if the truncated hardware Y would actually
       * draw here. Ordinary off-line sprites need no copy or second decode. */
      if(((line-1-((words>>8)&255))&255)>=height)continue;
      if(ppu!=&combat_ppu){memcpy(&combat_ppu,ppu,sizeof(combat_ppu));ppu=&combat_ppu;}
      ppu->oam[i]=(((line+64)&255)<<8)|128;ppu->highOam[i>>3]|=1<<(i&7);clipped=1;
    }
    if(clipped){ClearBackdrop(&ppu->objBuffer);ppu->lineHasSprites=ppu_evaluateSprites(ppu,line-1);}
  }
  if(clipped && line<=32){
    /* The native HUD is copied into both layouts. Rebuild its pixels as well,
     * otherwise an off-bottom sprite could survive through that separate copy. */
    uint8_t *original=ppu->renderBuffer;size_t pitch=ppu->renderPitch;
    ppu->renderBuffer=sm_scene_pixels;ppu->renderPitch=1024;
    sm_draw_full_line(ppu,line);
    memcpy(original+(line-1)*pitch,dst,1024);
    ppu->renderBuffer=original;ppu->renderPitch=pitch;
  }
  int timer_ui=0;
  if(((sm_scene_active && timer_status && line>32) || (!sm_scene_active && sm_presentation_kind()==2)) && !ppu->forcedBlank) {
    for(int x=0;x<256;x++) {
      int i=sm_last_sprite_owner[x];
      if(i==255 || (sm_last_main_layer[x]!=4 && sm_last_main_layer[x]!=6))continue;
      if(sm_sprite_view_gui(i,(uint32_t)ppu->oam[i]|(uint32_t)ppu->oam[i+1]<<16)) {
        uint8_t *ui=sm_ui_pixels+((line-1)*256+x)*4;
        memcpy(ui,dst+x*4,4);ui[3]=255;timer_ui=1;
      }
    }
    if(timer_ui) {
      if(ppu!=&combat_ppu)memcpy(&combat_ppu,ppu,sizeof(combat_ppu));
      ppu=&combat_ppu;
      for(int i=0;i<256;i+=2)if(sm_sprite_view_gui(i,(uint32_t)ppu->oam[i]|(uint32_t)ppu->oam[i+1]<<16)) {
        ppu->oam[i]=(ppu->oam[i]&0xff00)|128;ppu->highOam[i>>3]|=1<<(i&7);
      }
      ClearBackdrop(&ppu->objBuffer);ppu->lineHasSprites=ppu_evaluateSprites(ppu,line-1);
    }
  }
  if((frontend || story_text || (!sm_scene_active && timer_ui)) && !ppu->forcedBlank) {
    if(ppu!=&combat_ppu)memcpy(&combat_ppu,ppu,sizeof(combat_ppu));
    if(story_text)combat_ppu.layer[2].mainScreenEnabled=combat_ppu.layer[2].subScreenEnabled=false;
    if(frontend) {
      // Keep the real BG1 labels, borders, badges and OAM controls untouched;
      // only the original BG2 planet/stars enter the atmosphere shader.
      combat_ppu.layer[0].mainScreenEnabled=combat_ppu.layer[0].subScreenEnabled=false;
      combat_ppu.layer[4].mainScreenEnabled=combat_ppu.layer[4].subScreenEnabled=false;
    }
    combat_ppu.renderBuffer=sm_scene_pixels;combat_ppu.renderPitch=1024;
    sm_draw_full_line(&combat_ppu,line);
  }
  if (!sm_scene_active || ppu->forcedBlank || (ppu->mode != 1 && ppu->mode != 7) || line <= 32) {sm_profile_ns[2]+=sm_clock_ns()-profile_start;sm_wide_profiled(ppu,line);return;}
  // Unreal supplies rain/fog. Remove only their BG3 presentation pass after
  // recording the exact reference; native HDMA, tiles, SRAM and GUI stay intact.
  const int replace_weather=sm_scene_weather && sm_scene_active &&
      ppu->mode==1 && (fx_type==10 || fx_type==12) && gameplay_BG3SC!=0x58;
  const bool main_bg3=ppu->layer[2].mainScreenEnabled,sub_bg3=ppu->layer[2].subScreenEnabled;
  if(replace_weather || replace_pb || message || timer_ui || clipped) {
    uint8_t *original=ppu->renderBuffer;size_t pitch=ppu->renderPitch;
    if(replace_weather || message)ppu->layer[2].mainScreenEnabled=ppu->layer[2].subScreenEnabled=false;
    ppu->renderBuffer=sm_scene_pixels;ppu->renderPitch=1024;
    sm_draw_full_line(ppu,line);
    ppu->renderBuffer=original;ppu->renderPitch=pitch;
  }
  for(int x=0; x<256; ++x) {
    int layer=sm_last_main_layer[x];
    meta[x*4] = layer; /* original foreground/background/sprite provenance */
    meta[x*4+3] = 255;
    if(ppu->mode==1 && (room_ptr==0x91f8 || (layer2_scroll_x>1 && layer2_scroll_y!=1)) &&
       (layer==1 || layer==5)) {
      // Landing Site sky/mountain/foliage boundaries from native HDMA table
      // 88:AEC1. Preserve its wind speeds; only add camera parallax per plane.
      int wy=layer1_y_pos+line;
      sm_scene_background_mask[(line-1)*256+x]=room_ptr==0x91f8?(wy<1168?64:(wy<1192?128:192)):255;
    }
    sm_scene_emission[((line-1)*256+x)*4+3]=(layer==4 || layer==6)?255:0;
    if ((layer < 2 || layer == 5) && ppu->mode == 7) {
      int rx=ppu->m7xFlip?255-x:x;
      int xp=((ppu->m7startX+ppu->m7matrix[0]*rx)>>8)&1023;
      int yp=((ppu->m7startY+ppu->m7matrix[2]*rx)>>8)&1023;
      meta[x*4+1]=ppu->vram[(yp>>3)*128+(xp>>3)]&255;
      int tile=meta[x*4+1];
      int index=ppu->vram[tile*64+(yp&7)*8+(xp&7)]>>8;
      if(tile_emits(ppu,tile,7,index)) mark(ppu,x,line-1,index);
      if(room_ptr==0xdf45 && game_state==8 &&
         ppu->m7matrix[0]==256 && ppu->m7matrix[3]==256 &&
         !ppu->m7matrix[1] && !ppu->m7matrix[2] &&
         (background_masks[tile] & (1ull<<((yp&7)*8+(xp&7))))) {
        sm_scene_background_mask[(line-1)*256+x]=255;
        if(sm_scene_parallax) {
        int ci=ppu->vram[SM_CERES_PATTERN_TILE*64+((yp+sm_scene_camera_y)&7)*8+((xp+sm_scene_camera_x)&7)]>>8;
        // Reuse original color math/window/fade calculation for this texel.
        // Restore VRAM and target immediately, before another pixel or game code.
        int address=tile*64+(yp&7)*8+(xp&7);
        uint16_t saved=ppu->vram[address];uint8_t *target=ppu->renderBuffer;
        size_t pitch=ppu->renderPitch;
        ppu->vram[address]=(saved&255)|(ci<<8);
        ppu->renderBuffer=sm_scene_pixels;ppu->renderPitch=1024;
        ppu_handlePixel(ppu,x,line);
        ppu->renderBuffer=target;ppu->renderPitch=pitch;ppu->vram[address]=saved;
        }
      }
      meta[x*4+2]=128; /* Mode 7: 8-bit indexed tiles, not 4bpp SNES tiles. */
    } else if (layer < 2) {
      int px=(x+ppu->bgLayer[layer].hScroll)&0x3ff;
      int py=(line+ppu->bgLayer[layer].vScroll)&0x3ff;
      int addr=ppu->bgLayer[layer].tilemapAdr+((py>>3)&31)*32+((px>>3)&31);
      if ((px&256)&&ppu->bgLayer[layer].tilemapWider) addr+=0x400;
      if ((py&256)&&ppu->bgLayer[layer].tilemapHigher) addr+=ppu->bgLayer[layer].tilemapWider?0x800:0x400;
      uint16_t tile=ppu->vram[addr&0x7fff];
      meta[x*4+1]=tile&255;
      meta[x*4+2]=((tile>>8)&3)|(((tile>>10)&7)<<2);
      int index=ppu_getPixelForBgLayer(ppu,x,line,layer,(tile&0x2000)!=0);
      int gfx=(ppu->bgLayer[layer].tileAdr+(tile&1023)*16)&32767;
      if(index && tile_emits(ppu,gfx,4,index&15)) mark(ppu,x,line-1,index);
    }
  }
  /* BG2 often uses a streamed 512px tilemap: small bounded offsets avoid
   * exposing the far side of its streaming ring. Never change BG1 or OAM. */
  if (sm_scene_parallax && (sm_scene_camera_x || sm_scene_camera_y) && ppu->mode == 1 && !sm_scene_finite_bg2(ppu)) {
    uint16_t scroll_x=ppu->bgLayer[1].hScroll,scroll_y=ppu->bgLayer[1].vScroll;
    uint8_t *original=ppu->renderBuffer;
    size_t pitch=ppu->renderPitch;
    int shift_x=sm_scene_camera_x,shift_y=sm_scene_camera_y;
    if(room_ptr==0x91f8) {
      int wy=layer1_y_pos+line;
      shift_x=wy<1168?shift_x/3:(wy<1192?shift_x*2/3:shift_x);
      shift_y=0; // Keep native HDMA band boundaries and streamed rows aligned.
    }
    ppu->bgLayer[1].hScroll=scroll_x+shift_x;
    ppu->bgLayer[1].vScroll=scroll_y+shift_y;
    ppu->renderBuffer=sm_scene_pixels;
    ppu->renderPitch=256*4;
    uint8_t saved_line[1024];memcpy(saved_line,dst,1024);
    sm_draw_full_line(ppu,line);
    for(int x=0;x<256;++x)if(meta[x*4]!=1 && meta[x*4]!=5)memcpy(dst+x*4,saved_line+x*4,4);
    ppu->renderBuffer=original;
    ppu->renderPitch=pitch;
    ppu->bgLayer[1].hScroll=scroll_x;
    ppu->bgLayer[1].vScroll=scroll_y;
  }
  ppu->layer[2].mainScreenEnabled=main_bg3;
  ppu->layer[2].subScreenEnabled=sub_bg3;
  sm_profile_ns[2]+=sm_clock_ns()-profile_start;
  sm_wide_profiled(ppu,line);
}

typedef struct { float x,y,r,g,b,radius,power; } SmLight;
static SmLight lights[128];
static int nlights;
static void add_light(float x,float y,float r,float g,float b,float radius,float power) {
  if(nlights<128) lights[nlights++]=(SmLight){x,y,r,g,b,radius,power};
}
void sm_scene_finish(void) {
  Ppu *p=g_snes->ppu;
  memset(sm_scene_lights,0,sizeof(sm_scene_lights));
  if(!sm_scene_active) return;
  nlights=0;
  /* OAM supplies actual tile identity, position, palette and flipping. Only
   * reviewed fire tiles emit; the winning framebuffer layer checks occlusion. */
  memset(hash_valid,0,sizeof(hash_valid));
  for(int i=0;i<256;i+=2) {
    int a=p->oam[i+1],high=(p->highOam[i>>3]>>(i&7))&3;
    int size=spriteSizes[p->objSize][high>>1];
    int ox=(p->oam[i]&255)|((high&1)<<8);if(ox>255)ox-=512;
    int oy=p->oam[i]>>8;
    sm_sprite_view_y(i,(uint32_t)p->oam[i]|(uint32_t)p->oam[i+1]<<16,&oy);
    if(ox>=256 || ox+size<=0)continue;
    for(int y=0;y<size;++y)for(int x=0;x<size;++x) {
      int sx=ox+x,sy=oy+y-1;
      if(sx<0||sx>=256||sy<32||sy>=224)continue;
      uint8_t *mask=sm_scene_emission+(sy*256+sx)*4;
      if(mask[3]!=255)continue;
      int tx=(a&0x4000)?size-1-x:x,ty=(a&0x8000)?size-1-y:y;
      int tile=(((a&255)/16+ty/8)*16)|(((a&15)+tx/8)&15);
      int addr=(((a&256)?p->objTileAdr2:p->objTileAdr1)+tile*16)&32767;

      int col=7-(tx&7),row=ty&7;
      int w=p->vram[(addr+row)&32767],w2=p->vram[(addr+row+8)&32767];
      int index=((w>>col)&1)|(((w>>(8+col))&1)<<1)|(((w2>>col)&1)<<2)|(((w2>>(8+col))&1)<<3);
      if(!index||!tile_emits(p,addr,4,index))continue;
      int pal=128+((a>>9)&7)*16+index;
      uint16_t c=p->cgram[pal]; uint8_t *pixel=p->renderBuffer+sy*p->renderPitch+sx*4;
      // Reject a different sprite covering this texel (allow 5-bit rounding).
      if(abs(pixel[2]-(c&31)*255/31)>9 || abs(pixel[1]-((c>>5)&31)*255/31)>9 || abs(pixel[0]-((c>>10)&31)*255/31)>9)continue;
      mark(p,sx,sy,pal);
    }
  }
  /* Group neighbouring emitting texels into spatial sources. Each 16px cell
   * has at most one light, avoiding brightness proportional to tile count. */
  for(int cy=2;cy<14;++cy)for(int cx=0;cx<16;++cx) {
    float xsum=0,ysum=0,r=0,g=0,b=0;int count=0;
    for(int y=cy*16;y<cy*16+16;++y)for(int x=cx*16;x<cx*16+16;++x) {
      uint8_t *e=sm_scene_emission+(y*256+x)*4;
      if(!(e[0]|e[1]|e[2]))continue;
      xsum+=x;ysum+=y;r+=e[2];g+=e[1];b+=e[0];++count;
    }
    if(count) add_light(xsum/count,ysum/count,r/count/255,g/count/255,b/count/255,58,0.85f);
  }
  for(int i=0;i<10;++i) if(projectile_bomb_instruction_ptr[i]) {
    int x=(int16_t)(projectile_x_pos[i]-layer1_x_pos),y=(int16_t)(projectile_y_pos[i]-layer1_y_pos);
    if(x<0||x>=256||y<32||y>=224)continue;
    int type=projectile_type[i]&0xf00;
    int explosion=type==0x700||type==0x800;
    add_light(x,y,1.f, type==0x200?.95f:.64f, .20f, explosion?70:38,explosion?1.3f:.95f);
  }
  // Lava/acid light follows the simulation's actual fluid height, not red art.
  if((fx_type==2 || fx_type==4) && !(lava_acid_y_pos&0x8000)) {
    int surface=(int16_t)(lava_acid_y_pos-layer1_y_pos);
    if(surface<256) for(int x=16;x<256;x+=32)
      add_light(x,surface<32?32:surface,1.f,fx_type==2?.32f:.64f,.06f,82,.35f);
  }
  for(int y=8;y<56;++y)for(int x=0;x<64;++x) {
    float c[3]={0};
    for(int i=0;i<nlights;++i) {
      SmLight *l=&lights[i];float dx=x*4+2-l->x,dy=y*4+2-l->y;
      float f=1-(dx*dx+dy*dy)/(l->radius*l->radius);
      if(f<=0)continue;f=f*f*l->power;
      c[0]+=l->b*f;c[1]+=l->g*f;c[2]+=l->r*f;
    }
    for(int k=0;k<3;++k)sm_scene_lights[(y*64+x)*4+k]=(uint8_t)(fminf(c[k],1.f)*255);
  }
}

void sm_set_widescreen(int enabled) {sm_wide_enabled=!!enabled;}
const uint8_t *sm_wide_scene(void) {return sm_wide_pixels;}
const uint8_t *sm_wide_metadata(void) {return sm_wide_layers;}
const uint8_t *sm_wide_background(void) {return sm_wide_far;}
const uint8_t *sm_wide_overlay(void) {return sm_wide_hud;}
int sm_wide_available(void) {if(sm_credits_state(0))return sm_wide_enabled;return wide_valid;}

int sm_view_margin(void) {return sm_wide_enabled && sm_scene_active && wide_valid?72:0;}

/* Room loading can reuse VRAM/CGRAM before the first destination frame.
 * Freeze only the HUD during those states; simulation and all menu art remain
 * live. Apply current fade so a cached HUD cannot linger over a black screen. */
void sm_hud_stabilize(uint8_t *pixels) {
  int b=g_snes->ppu->forcedBlank?0:g_snes->ppu->brightness;
  if(game_state==8 && !sm_message_active() && b==15) {
    memcpy(stable_hud,pixels,sizeof(stable_hud));
    memcpy(stable_wide_hud,sm_wide_hud,sizeof(stable_wide_hud));stable_hud_brightness=b;
  } else if(game_state>=9 && game_state<=11 && stable_hud_brightness) {
    for(unsigned i=0;i<sizeof(stable_hud);i++) {
      uint8_t v=i%4==3?stable_hud[i]:stable_hud[i]*b/stable_hud_brightness;
      pixels[i]=sm_scene_pixels[i]=v;
    }
    for(unsigned i=0;i<sizeof(stable_wide_hud);i++)
      sm_wide_hud[i]=i%4==3?stable_wide_hud[i]:stable_wide_hud[i]*b/stable_hud_brightness;
  } else if(game_state<6 || game_state>18)stable_hud_brightness=0;
}
