/* Decorative continuation using only room-authored metatiles. This does not
 * alter level_data, BTS, PLMs, collision or the game's random stream. */
static int border_enabled=1,border_samples,border_missing;
static struct {int bx,by,layer,used;uint16_t block;} border_cache[1024];
static uint8_t border_visible_cache[2][1024];
void sm_set_border_extension(int enabled){border_enabled=!!enabled;}
int sm_border_stat(int field){return field?border_missing:border_samples;}
static uint16_t border_block(int layer,int x,int y) {
  int addr=0x10002+2*(y*room_width_in_blocks+x)+(layer?0x9600:0);
  return addr>=0 && addr+1<0x20000?GET_WORD(g_ram+addr):0;
}
static int border_visible(uint16_t block,int layer) {
  int id=block&1023;
  if(border_visible_cache[layer][id])return border_visible_cache[layer][id]==2;
  const uint16_t *tiles=(const uint16_t*)(g_ram+0xa000+id*8);
  Ppu *p=g_snes->ppu;
  for(int i=0;i<4;i++) {
    int t=tiles[i],address=p->bgLayer[layer].tileAdr+(t&1023)*16,pal=((t>>10)&7)*16;
    for(int y=0;y<8;y++) {
      uint16_t lo=p->vram[(address+y)&32767],hi=p->vram[(address+y+8)&32767];
      for(int x=0;x<8;x++) {
        int c=((lo>>x)&1)|((lo>>(x+7))&2)|((hi>>x)&1)*4|((hi>>(x+7))&2)*4;
        if(c && p->cgram[pal+c]){border_visible_cache[layer][id]=2;return 1;}
      }
    }
  }
  border_visible_cache[layer][id]=1;return 0;
}
static int border_plain(uint16_t block,int layer) {
  /* Only ordinary solid foreground. Door/extension/destructible blocks never
   * become decorative candidates. BG2 has no collision type interpretation. */
  return (layer || (block>>12)==8) && border_visible(block,layer);
}
static uint16_t border_choose(int layer,int bx,int by) {
  int w=room_width_in_blocks,h=room_height_in_blocks;
  if(w<=0 || h<=0 || w*h>0x4b00)return 0;
  unsigned slot=((unsigned)bx*73856093u^(unsigned)by*19349663u^layer*83492791u)&1023;
  for(int n=0;n<1024;n++,slot=(slot+1)&1023) {
    if(!border_cache[slot].used)break;
    if(border_cache[slot].bx==bx && border_cache[slot].by==by && border_cache[slot].layer==layer)
      return border_cache[slot].block;
  }
  int cx=IntMax(0,IntMin(w-1,bx)),cy=IntMax(0,IntMin(h-1,by));
  uint16_t edge=border_block(layer,cx,cy),result=edge;
  int best=0x7fffffff;
  /* Prefer the nearest authored continuation along the same row/column.
   * A tile matching its inward neighbour is a fill, rather than a cap with a
   * visible seam. Keep complete 16px motifs and their native flip bits. */
  for(int y=IntMax(0,cy-5);y<=IntMin(h-1,cy+5);y++)
    for(int x=IntMax(0,cx-5);x<=IntMin(w-1,cx+5);x++) {
      uint16_t b=border_block(layer,x,y);
      if(!border_plain(b,layer))continue;
      int score=(abs(x-cx)+abs(y-cy))*12;
      if(bx<0 || bx>=w) {
        score+=abs(y-cy)*10;
        int ix=x+(bx<0?1:-1);
        if(ix>=0&&ix<w && (border_block(layer,ix,y)&0xfff)==(b&0xfff))score-=20;
      }
      if(by<0 || by>=h) {
        score+=abs(x-cx)*10;
        int iy=y+(by<0?1:-1);
        if(iy>=0&&iy<h && (border_block(layer,x,iy)&0xfff)==(b&0xfff))score-=20;
      }
      if((b&0xc00)==(edge&0xc00))score-=2;
      if(score<best){best=score;result=b;}
    }
  if(best==0x7fffffff)++border_missing;
  else ++border_samples;
  border_cache[slot].bx=bx;border_cache[slot].by=by;border_cache[slot].layer=layer;
  border_cache[slot].block=result;border_cache[slot].used=1;
  return result;
}
