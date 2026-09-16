/* Widen the actual pause PPU output. Native routines retain ownership of
 * navigation, map exploration, icons, animation, equipment and reserve use.
 * Only a presentation copy of the PPU is touched. */
static int pause_art_source_x(int x) {
  if(x<96)return x;
  if(x<168)return 88+(x&7);
  if(x<232)return x-72;
  if(x<304)return 160+(x&7);
  return x-144;
}
static int pause_art_target_x(int x) {
  return x<96?x:(x<160?x+72:x+144);
}
static void wide_pause_line(Ppu *original,int line) {
  const int y=line-1,map=pause_screen_mode==0;
  const uint8_t *native=original->renderBuffer+y*original->renderPitch;
  uint8_t *out=sm_wide_hud+y*400*4;
  /* Keep every original panel pixel at 1:1. Added spans use the native BG2
   * grid, not an invented frame, font, palette, or duplicated equipment. */
  for(int x=0;x<400;x++) {
    int sx=pause_art_source_x(x);
    if(y<48 && x>=96 && x<168)sx=64+(x&7);
    if(y<48 && x>=232 && x<304)sx=184+(x&7);
    /* Keep the entire native title together (including long area names).
     * Extend straight frame tiles instead of retaining the old title gaps. */
    if(y>=32 && y<48 && x>=32 && x<368)
      sx=x>=136 && x<264?x-72:32+(x&7);
    memcpy(out+x*4,native+sx*4,4);out[x*4+3]=255;
  }
  if(y<48 || y>=192)return;
  if(pause_screen_mode==2){
    /* Reflow original objective tiles into the wider native frame. Glyphs,
     * counters and completion marks remain 8x8; no split strings or scaling. */
    for(int start=0;start<400;start+=200){
      Ppu *p=&wide_ppu;memcpy(p,original,sizeof(*p));
      p->renderBuffer=wide_line_buffer;p->renderPitch=1024;
      memcpy(p->vram+0x3000,sm_objective_pause_wide(),4096);
      p->layer[2].mainScreenEnabled=p->layer[2].subScreenEnabled=false;
      p->layer[4].mainScreenEnabled=p->layer[4].subScreenEnabled=false;
      for(int local=0;local<200;local++){
        int x=start+local;
        p->bgLayer[0].hScroll=start;
        p->bgLayer[1].hScroll=original->bgLayer[1].hScroll+pause_art_source_x(x)-local;
        for(int k=0;k<5;k++)p->layer[k].mainScreenWindowed=p->layer[k].subScreenWindowed=false;
        ppu_handlePixel(p,local,line);
        memcpy(out+x*4,wide_line_buffer+y*1024+local*4,4);out[x*4+3]=255;
      }
    }
    return;
  }
  for(int start=0;start<400;start+=200) {
    Ppu *p=&wide_ppu;memcpy(p,original,sizeof(*p));
    p->renderBuffer=wide_line_buffer;p->renderPitch=1024;
    p->layer[2].mainScreenEnabled=p->layer[2].subScreenEnabled=false;
    /* Map icons use their full authored coordinates, even beyond the SNES
     * viewport. Do not wrap distant icons into the expanded map. */
    for(int i=0;i<256;i+=2) {
      int shift=i&7,hi=(p->highOam[i>>3]>>shift)&1;
      int sx=(p->oam[i]&255)|(hi<<8),full;
      if(sx&256)sx-=512;
      if(sm_sprite_view_x(i,(uint32_t)p->oam[i]|(uint32_t)p->oam[i+1]<<16,&full))sx=full;
      int sy=p->oam[i]>>8;
      int arrow=sm_sprite_view_gui(i,(uint32_t)p->oam[i]|(uint32_t)p->oam[i+1]<<16);
      int dx=map && sy>=48 && sy<192 && !arrow?sx+72:pause_art_target_x(sx);
      int size=spriteSizes[p->objSize][(p->highOam[i>>3]>>(shift+1))&1];
      dx=dx+size<=start || dx>=start+200?384:((dx-start)&511);
      p->oam[i]=(p->oam[i]&0xff00)|(dx&255);
      p->highOam[i>>3]=(p->highOam[i>>3]&~(1<<shift))|((dx>>8)<<shift);
    }
    ClearBackdrop(&p->objBuffer);p->lineHasSprites=ppu_evaluateSprites(p,line-1);
    for(int local=0;local<200;local++) {
      int x=start+local,gap=(x>=96&&x<168)||(x>=232&&x<304);
      if(map?(x<8||x>=392):!gap)continue;
      /* The grid comes from the same live BG2 tilemap as the native pause.
       * In equipment gaps, omit BG1 and OBJ to avoid duplicating any panel. */
      p->layer[0].mainScreenEnabled=map && original->layer[0].mainScreenEnabled;
      p->layer[4].mainScreenEnabled=map && original->layer[4].mainScreenEnabled;
      p->layer[0].subScreenEnabled=p->layer[4].subScreenEnabled=false;
      p->bgLayer[0].hScroll=original->bgLayer[0].hScroll+start-72;
      p->bgLayer[1].hScroll=original->bgLayer[1].hScroll+pause_art_source_x(x)-local;
      int map_x=(int16_t)reg_BG1HOFS+x-72;
      if(map_x<0 || map_x>=512)p->layer[0].mainScreenEnabled=false;
      for(int k=0;k<5;k++)p->layer[k].mainScreenWindowed=p->layer[k].subScreenWindowed=false;
      ppu_handlePixel(p,local,line);
      memcpy(out+x*4,wide_line_buffer+y*1024+local*4,4);out[x*4+3]=255;
    }
  }
}
