#pragma once
/* All shapes/palettes come from validated ROM tiles. Composition metadata only. */
typedef struct {uint16_t gfx,offset,palette,palette_offset;int16_t x,y;uint8_t flip;} SmTrackerTile;
#include "sm_tracker_rom_assets.inc"
void sm_tracker_load_assets(void){
  enum {COUNT=sizeof(tracker_sources)/sizeof(*tracker_sources)};
  const uint8_t *sources[COUNT];uint8_t *allocated[COUNT]={0};
  for(unsigned i=0;i<COUNT;i++){
    if(tracker_sources[i].compressed){
      allocated[i]=malloc(65536);
      if(!allocated[i]){
        for(unsigned j=0;j<i;j++)free(allocated[j]);
        Die("Cannot allocate ROM tracker graphics");return;
      }
      DecompressToMem(tracker_sources[i].address,allocated[i]);sources[i]=allocated[i];
    }else sources[i]=RomFixedPtr(tracker_sources[i].address);
  }
  memset(tracker_icons,0,sizeof(tracker_icons));
  for(unsigned id=0;id<26;id++){
    for(unsigned j=0;j<tracker_compositions[id].count;j++){
      const SmTrackerTile *op=&tracker_tiles[tracker_compositions[id].first+j];
      const uint8_t *gfx=sources[op->gfx]+op->offset;
      const uint8_t *pal=sources[op->palette]+op->palette_offset;
      for(int y=0;y<16;y++)for(int x=0;x<16;x++){
        int sx=tracker_compositions[id].x+(2*x+1)*tracker_compositions[id].size/32-op->x;
        int sy=tracker_compositions[id].y+(2*y+1)*tracker_compositions[id].size/32-op->y;
        if(sx<0 || sx>=8 || sy<0 || sy>=8)continue;
        if(op->flip&1)sx=7-sx;if(op->flip&2)sy=7-sy;
        unsigned ci=0;for(int b=0;b<4;b++)ci|=((gfx[sy*2+b/2*16+b%2]>>(7-sx))&1)<<b;
        if(!ci)continue;
        uint16_t color=GET_WORD(pal+ci*2);uint8_t *pixel=tracker_icons[id]+(y*16+x)*4;
        for(int c=0;c<3;c++)pixel[c]=((color>>(c*5))&31)<<3;
        pixel[3]=255;
      }
    }
  }
  for(unsigned i=0;i<COUNT;i++)free(allocated[i]);
}
static uint8_t window_icon[16*16*4];
const uint8_t *sm_window_icon_pixels(void){
  /* Same unmodified helmet and palette as the native file-slot emblem. */
  const uint8_t *pal=RomFixedPtr(0x8ee400)+240*2;
  for(int y=0;y<16;y++)for(int x=0;x<16;x++){
    const uint8_t *gfx=RomFixedPtr(0xb6c000)+(0xd0+x/8+y/8*16)*32;
    unsigned ci=0;for(int b=0;b<4;b++)ci|=((gfx[(y&7)*2+b/2*16+b%2]>>(7-(x&7)))&1)<<b;
    uint16_t color=GET_WORD(pal+ci*2);uint8_t *out=window_icon+(y*16+x)*4;
    for(int c=0;c<3;c++)out[c]=((color>>(c*5))&31)*255/31;
    out[3]=ci?255:0;
  }
  return window_icon;
}
