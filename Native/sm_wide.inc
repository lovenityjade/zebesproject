/* Included in sm_scene.c to reuse the exact tile/color decoder. The copy owns
 * all temporary scroll, VRAM, OAM and line-buffer changes. Native PPU state is
 * never modified by the wide view. 400x224, integer-scaled in Unreal. */
#include "sm_sprite_view.h"
int sm_wide_enabled;
uint8_t sm_wide_pixels[400*240*4],sm_wide_layers[400*240*4];
uint8_t sm_wide_far[400*240],sm_wide_hud[400*240*4];
static Ppu wide_ppu,hud_ppu;
static uint8_t wide_line_buffer[256*240*4];
static int wide_valid;
static int wide_blank_tiles[2];
#include "sm_border.inc"
#include "sm_pause_wide.inc"
static uint16_t wide_room_tile(int layer,int wx,int wy) {
  int outside=wx<0 || wy<0 || wx>=room_width_in_blocks*16 || wy>=room_height_in_blocks*16;
  int bx=wx>=0?wx/16:-((-wx+15)/16),by=wy>=0?wy/16:-((-wy+15)/16);
  uint16_t painted=0;int authored=!layer && sm_decor_get(0,bx,by,&painted);
  if(outside && !border_enabled && !authored) {
    if(wide_blank_tiles[layer]<0) {
      Ppu *p=g_snes->ppu;wide_blank_tiles[layer]=0;
      for(int t=0;t<1024;t++) {
        int blank=1;for(int i=0;i<16;i++)if(p->vram[(p->bgLayer[layer].tileAdr+t*16+i)&32767]){blank=0;break;}
        if(blank){wide_blank_tiles[layer]=t;break;}
      }
    }
    return wide_blank_tiles[layer];
  }
  int index=by*room_width_in_blocks+bx;
  int addr=0x10002+index*2+(layer?0x9600:0);
  if(!outside && (addr<0 || addr+1>=0x20000))return 0;
  uint16_t block=authored?painted:(outside?border_choose(layer,bx,by):GET_WORD(g_ram+addr));
  /* Original solid black tiles were hidden by the 32px HUD or camera limits.
   * Continue their material only in the newly exposed presentation spans. */
  if(!authored && border_enabled && !outside && !layer && (block>>12)==8 && !border_visible(block,0))
    block=border_choose(0,bx,by);
  int tx=(wx>>3)&1,ty=(wy>>3)&1;
  if(block&0x400)tx^=1;if(block&0x800)ty^=1;
  const uint16_t *tiles=(const uint16_t*)(g_ram+0xa000+(block&1023)*8);
  return tiles[ty*2+tx]^((block&0xc00)<<4);
}
static void wide_paint_background(Ppu *p,int line,int offset,int count) {
  if(!sm_decor_room_active())return;
  const BgLayer *b=&p->bgLayer[1];
  for(int x=offset-8;x<offset+count+8;x+=8) {
    int wx=layer1_x_pos+x,wy=layer1_y_pos+line;
    int bx=wx>=0?wx/16:-((-wx+15)/16),by=wy>=0?wy/16:-((-wy+15)/16);
    uint16_t block;if(!sm_decor_get(1,bx,by,&block))continue;
    int tx=(wx>>3)&1,ty=(wy>>3)&1;
    if(block&0x400)tx^=1;if(block&0x800)ty^=1;
    const uint16_t *tiles=(const uint16_t*)(g_ram+0xa000+(block&1023)*8);
    int px=(x+b->hScroll)&511,py=(line+b->vScroll)&511;
    int address=b->tilemapAdr+((py>>3)&31)*32+((px>>3)&31);
    if((px&256)&&b->tilemapWider)address+=0x400;
    if((py&256)&&b->tilemapHigher)address+=b->tilemapWider?0x800:0x400;
    p->vram[address&32767]=tiles[ty*2+tx]^((block&0xc00)<<4);
  }
}
static void wide_fill_row(Ppu *p,int layer,int line,int offset,int count) {
  const BgLayer *b=&p->bgLayer[layer];
  int camera_x=layer?layer2_x_pos:layer1_x_pos;
  int camera_y=layer?layer2_y_pos:layer1_y_pos;
  int delta_y=(int16_t)(b->vScroll-camera_y);
  int wy=camera_y+line;
  // VRAM tilemap scroll includes the room's VRAM ring offset. The tile data
  // itself is addressed in world coordinates, not by the wrapped VRAM index.
  for(int x=offset-8;x<offset+count+8;x+=8) {
    int wx=camera_x+x;
    int px=(x+b->hScroll)&511,py=(line+b->vScroll)&511;
    int addr=b->tilemapAdr+((py>>3)&31)*32+((px>>3)&31);
    if((px&256)&&b->tilemapWider)addr+=0x400;
    if((py&256)&&b->tilemapHigher)addr+=b->tilemapWider?0x800:0x400;
    (void)delta_y;
    p->vram[addr&32767]=wide_room_tile(layer,wx,wy);
  }
}
static void wide_draw_span(Ppu *original,int line,int offset,int count,int destination) {
  Ppu *p=&wide_ppu;memcpy(p,original,sizeof(*p));
  if(sm_combat_effects && ((power_bomb_explosion_status&0x8000) || sm_visual_eye())) {memset(p->mathEnabled,0,sizeof(p->mathEnabled));p->clipMode=0;}
  // Rebuild the streamed maps on the copy. Animated/destructible blocks have
  // already updated level_data, so the side view follows their current state.
  // During the middle of a door load the RAM room, VRAM and scroll registers
  // describe different rooms. Only reconstruct once that data is coherent.
  int room_ready=game_state!=11 || door_transition_function<0xe36e || door_transition_function==0xe737;
  if(p->mode==1 && room_ready) {
    wide_fill_row(p,0,line,offset,count);
    if(!(layer2_scroll_x&1) && !(layer2_scroll_y&1)) {
      wide_fill_row(p,1,line,offset,count);
      if(sm_scene_parallax && (sm_scene_camera_x || sm_scene_camera_y)) {
        int sx=sm_scene_camera_x,sy=sm_scene_camera_y,wy=layer1_y_pos+line;
        if(room_ptr==0x91f8){sx=wy<1168?sx/3:(wy<1192?sx*2/3:sx);sy=0;}
        wide_fill_row(p,1,line+sy,offset+sx,count);
      }
    }
  } else if(p->mode==7) {
    ppu_calculateMode7Starts(p,line);
    int direction=p->m7xFlip?-1:1;
    p->m7startX+=p->m7matrix[0]*offset*direction;
    p->m7startY+=p->m7matrix[2]*offset*direction;
  }
  if(p->mode==1 && room_ready)wide_paint_background(p,line,offset,count);
  if(line<=32 || sm_message_active() || (sm_scene_weather && (fx_type==10 || fx_type==12) && gameplay_BG3SC!=0x58))
    p->layer[2].mainScreenEnabled=p->layer[2].subScreenEnabled=false;
  for(int l=0;l<4;l++)p->bgLayer[l].hScroll+=offset;
  for(int i=0;i<256;i+=2) {
    int shift=i&7,hi=(p->highOam[i>>3]>>shift)&1;
    int x=(((p->oam[i]&255)|(hi<<8))-offset)&511;
    int full_x;
    if(sm_sprite_view_x(i,(uint32_t)p->oam[i]|(uint32_t)p->oam[i+1]<<16,&full_x) &&
       (full_x&511)==((p->oam[i]&255)|(hi<<8))) {
      int size=spriteSizes[p->objSize][(p->highOam[i>>3]>>(shift+1))&1];
      x=full_x+size<=offset || full_x>=offset+count ? 384 : (full_x-offset)&511;
    }
    int full_y;
    if(sm_sprite_view_y(i,(uint32_t)p->oam[i]|(uint32_t)p->oam[i+1]<<16,&full_y)) {
      int height=spriteSizes[p->objSize][(p->highOam[i>>3]>>(shift+1))&1];
      if(line-1<full_y || line-1>=full_y+height)x=384;
    }
    if(timer_status && sm_sprite_view_gui(i,(uint32_t)g_snes->ppu->oam[i]|(uint32_t)g_snes->ppu->oam[i+1]<<16))x=384;
    p->oam[i]=(p->oam[i]&0xff00)|(x&255);
    p->highOam[i>>3]=(p->highOam[i>>3]&~(1<<shift))|((x>>8)<<shift);
  }
  ClearBackdrop(&p->objBuffer);p->lineHasSprites=ppu_evaluateSprites(p,line-1);
  p->renderBuffer=wide_line_buffer;p->renderPitch=1024;
  // Window boundaries describe original display coordinates. Their effect is
  // extended at the edges, not wrapped into the newly visible portions.
  for(int i=0;i<6;i++) {
    if(offset && (p->windowLayer[i].window1enabled||p->windowLayer[i].window2enabled)) {
      p->window1left=IntMax(0,IntMin(255,(int)original->window1left-offset));
      p->window1right=IntMax(0,IntMin(255,(int)original->window1right-offset));
      p->window2left=IntMax(0,IntMin(255,(int)original->window2left-offset));
      p->window2right=IntMax(0,IntMin(255,(int)original->window2right-offset));
    }
  }
  const int fast=sm_fast_line_supported(p);
  if(fast)sm_draw_full_line(p,line);
  for(int x=0;x<count;x++) {
    int idx=(line-1)*400+destination+x,wx=(int)layer1_x_pos+offset+x;
    uint8_t *out=sm_wide_pixels+idx*4,*meta=sm_wide_layers+idx*4;
    int wy=(int)layer1_y_pos+line;
    int outside=wx<0 || wx>=room_width_in_blocks*16 || wy<0 || wy>=room_height_in_blocks*16;
    int bx=wx>=0?wx/16:-((-wx+15)/16),by=wy>=0?wy/16:-((-wy+15)/16);uint16_t authored;
    int painted=outside && (sm_decor_get(0,bx,by,&authored)||sm_decor_get(1,bx,by,&authored));
    if(!room_ready || (outside && (!border_enabled || p->mode!=1) && !painted)) {memset(out,0,4);out[3]=255;memset(meta,0,4);continue;}
    if(!fast)ppu_handlePixel(p,x,line);
    int layer=sm_last_main_layer[x];
    meta[0]=layer;meta[1]=meta[2]=0;meta[3]=255;
    if(p->mode==1 && (layer==1 || layer==5)) {
      int wy=layer1_y_pos+line;
      sm_wide_far[idx]=room_ptr==0x91f8?(wy<1168?64:(wy<1192?128:192)):255;
      if(sm_scene_parallax && (sm_scene_camera_x || sm_scene_camera_y)) {
        int sx=sm_scene_camera_x,sy=sm_scene_camera_y;
        if(room_ptr==0x91f8){sx=wy<1168?sx/3:(wy<1192?sx*2/3:sx);sy=0;}
        uint16_t hx=p->bgLayer[1].hScroll,hy=p->bgLayer[1].vScroll;
        p->bgLayer[1].hScroll=hx+sx;p->bgLayer[1].vScroll=hy+sy;
        ppu_handlePixel(p,x,line);
        p->bgLayer[1].hScroll=hx;p->bgLayer[1].vScroll=hy;
      }
    } else if(p->mode==7 && (layer<2 || layer==5)) {
      int rx=p->m7xFlip?255-x:x;
      int xp=((p->m7startX+p->m7matrix[0]*rx)>>8)&1023;
      int yp=((p->m7startY+p->m7matrix[2]*rx)>>8)&1023;
      int tile=p->vram[(yp>>3)*128+(xp>>3)]&255;
      meta[1]=tile;meta[2]=128;tile_hash(p,tile,7);
      if(room_ptr==0xdf45 && p->m7matrix[0]==256 && p->m7matrix[3]==256 &&
         !p->m7matrix[1] && !p->m7matrix[2] &&
         (background_masks[tile]&(1ull<<((yp&7)*8+(xp&7))))) {
        sm_wide_far[idx]=255;
        if(sm_scene_parallax && (sm_scene_camera_x || sm_scene_camera_y)) {
          int address=tile*64+(yp&7)*8+(xp&7);uint16_t saved=p->vram[address];
          int ci=p->vram[SM_CERES_PATTERN_TILE*64+((yp+sm_scene_camera_y)&7)*8+((xp+sm_scene_camera_x)&7)]>>8;
          p->vram[address]=(saved&255)|(ci<<8);ppu_handlePixel(p,x,line);p->vram[address]=saved;
        }
      }
    }
    memcpy(out,wide_line_buffer+(line-1)*1024+x*4,4);out[3]=255;
  }
}
static void wide_map(void) {
  if(area_index>=7 || debug_disable_minimap)return;
  const uint8_t *ptr=RomFixedPtr(0x82964a)+area_index*3;
  const uint16_t *map=(const uint16_t*)RomPtr(ptr[0]|ptr[1]<<8|ptr[2]<<16);
  const uint8_t *known=sm_seed_map_data(area_index);
  int mx=room_x_coordinate_on_map+(samus_x_pos>>8),my=room_y_coordinate_on_map+(samus_y_pos>>8)+1;
  for(int y=0;y<4;y++)for(int x=0;x<7;x++) {
    int gx=mx+x-4,gy=my+y-1;uint16_t tile=0x2c1f;
    if(gx>=0&&gx<64&&gy>=0&&gy<32) {
      int bit=((gx&31)>>3)+4*((gx&32)+gy),mask=0x80>>(gx&7);
      int index=(gx&31)+gy*32+(gx&32)*32;
      if(map_tiles_explored[bit]&mask)tile=(map[index]&0xc3ff)|0x2800;
      else if((sm_map_fully_known()||has_area_map)&&(known[bit]&mask))tile=(map[index]&0xc3ff)|0x2c00;
    }
    if(x==4 && y==1 && !(nmi_frame_counter_byte&8))tile|=0x1c00;
    int address=hud_ppu.bgLayer[2].tileAdr+(tile&1023)*8;
    for(int py=0;py<8;py++)for(int px=0;px<8;px++) {
      int row=(tile&0x8000)?7-py:py,col=(tile&0x4000)?px:7-px;
      uint16_t planes=hud_ppu.vram[(address+row)&32767];
      int color=((planes>>col)&1)|((planes>>(col+7))&2);
      uint16_t c=hud_ppu.cgram[((tile>>10)&7)*4+color];
      uint8_t *out=sm_wide_hud+((y*8+py)*400+336+x*8+px)*4;
      out[0]=hud_ppu.brightnessMult[(c>>10)&31];out[1]=hud_ppu.brightnessMult[(c>>5)&31];out[2]=hud_ppu.brightnessMult[c&31];
      out[3]=(out[0]|out[1]|out[2])?255:0;
    }
  }
}
static void wide_hud_line(Ppu *p,int line) {
  if(line==32)return; // The native solid separator is not part of any HUD group.
  const uint8_t *source=p->renderBuffer+(line-1)*p->renderPitch;
  for(int x=0;x<208;x++) {
    // Energy/AUTO reserves occupy columns 0..9. Weapons start at column 10.
    uint8_t *out=sm_wide_hud+((line-1)*400+x+(x>=80?72:0))*4;
    memcpy(out,source+x*4,4);out[3]=(out[0]|out[1]|out[2])?255:0;
  }
}
static void sm_wide_line(Ppu *p,int line) {
  if(!sm_wide_enabled && !sm_decor_room_active())return;
  if(line==1) {
    memset(border_cache,0,sizeof(border_cache));border_samples=border_missing=0;
    memset(border_visible_cache,0,sizeof(border_visible_cache));
    wide_blank_tiles[0]=wide_blank_tiles[1]=-1;
    memset(sm_wide_pixels,0,sizeof(sm_wide_pixels));memset(sm_wide_hud,0,sizeof(sm_wide_hud));
    memset(sm_wide_layers,0,sizeof(sm_wide_layers));memset(sm_wide_far,0,sizeof(sm_wide_far));
    memcpy(&hud_ppu,p,sizeof(*p));wide_valid=0;
  }
  if(line<1 || line>224)return;
  wide_valid=1; // Fixed canvas, including blank frames, menus and cinematics.
  if(!sm_scene_active || p->forcedBlank || (p->mode!=1 && p->mode!=7)) {
    // The pause map/equipment pages share the same native top HUD. Preserve
    // its anchors and expanded minimap while centering the page underneath.
    int pause_hud=game_state>=13 && game_state<=17 && !sm_map_browser_overview() && !p->forcedBlank && p->mode==1;
    if(pause_hud && line<=32) {wide_hud_line(p,line);return;}
    if(pause_hud && line==33)wide_map();
    if(pause_hud && p->bgLayer[0].tilemapAdr==0x3000 && p->bgLayer[1].tilemapAdr==0x3800) {
      wide_pause_line(p,line);return;
    }
    memcpy(sm_wide_pixels+((line-1)*400+72)*4,sm_scene_pixels+(line-1)*1024,1024);
    memcpy(sm_wide_hud+((line-1)*400+72)*4,sm_ui_pixels+(line-1)*1024,1024);
    return;
  }
  if(line<=32) {
    wide_hud_line(p,line);
    return;
  }
  if(p->mode!=1 && p->mode!=7)return;
  wide_valid=1;
  if(line==33) {
    for(int y=1;y<=32;y++) {
      wide_draw_span(p,y,-72,200,0);wide_draw_span(p,y,128,200,200);
    }
    wide_map();
  }
  int row=line-1;
  memcpy(sm_wide_pixels+(row*400+72)*4,sm_scene_pixels+row*1024,1024);
  memcpy(sm_wide_layers+(row*400+72)*4,sm_scene_layers+row*1024,1024);
  memcpy(sm_wide_far+row*400+72,sm_scene_background_mask+row*256,256);
  memcpy(sm_wide_hud+(row*400+72)*4,sm_ui_pixels+row*1024,1024);
  if(sm_decor_room_active() && p->mode==1) {
    wide_draw_span(p,line,-72,200,0);wide_draw_span(p,line,128,200,200);return;
  }
  wide_draw_span(p,line,-72,72,0);wide_draw_span(p,line,256,72,328);
}
