#pragma once
/* Decode an original ROM tile, changing only palette-slot assignments. */
typedef struct {uint16_t tile;uint32_t source;uint8_t colors[16];} SmUiRomTile;
static void sm_ui_rom_tile(uint8_t *out,int depth,const SmUiRomTile *tile){
  const uint8_t *source=RomFixedPtr(tile->source);
  uint8_t *target=out+tile->tile*depth*8;
  memset(target,0,depth*8);
  for(int y=0;y<8;y++)for(int x=0;x<8;x++){
    unsigned color=0;
    for(int p=0;p<depth;p++)color|=((source[y*2+p/2*16+p%2]>>(7-x))&1)<<p;
    color=tile->colors[color];
    for(int p=0;p<depth;p++)target[y*2+p/2*16+p%2]|=((color>>p)&1)<<(7-x);
  }
}
