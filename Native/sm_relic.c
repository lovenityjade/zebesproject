#include "sm_locale.h"
#include "sm_relic.h"
#include "sm_seed.h"
#include "sm_generation.h"
#include "sm_map_browser.h"
#include "sm_effects.h"
#include "sm_run_stats.h"
#include "ida_types.h"
#include "variables.h"
#include "sm_rtl.h"
#include "funcs.h"
#include <string.h>
#include <stdio.h>
static uint8_t quotas[3],blank_gfx[4];
static int reloading,pending_spark,departing,spark_x,spark_y;
static int pickup_message,pickup_count,pickup_quota,warning_message;
static const uint32_t sprite[256]={
#include "sm_relic_pixels.inc"
};
extern uint8_t sm_scene_pixels[],sm_wide_pixels[],sm_wide_hud[];
void sm_relic_new_session(void){sm_relic_escape_session();memset(blank_gfx,0,4);reloading=pending_spark=departing=pickup_message=warning_message=0;}
void sm_relic_reset(void){memset(quotas,0,3);sm_relic_new_session();}
void sm_relic_configure(int slot,int required){if(slot>=0 && slot<3 && required>=0 && required<=60)quotas[slot]=required;}
int sm_relic_required(void){int slot=sm_slots_current();return sm_seed_active() && slot>=0 && slot<3?quotas[slot]:0;}
int sm_relic_count(void){int count=0;for(int i=0;i<100;i++)count+=sm_seed_item_kind(i)==1 && sm_seed_location_collected(i);return count;}
void sm_relic_reload(int active){reloading=active;}
uint16_t sm_relic_gfx(uint16_t plm,uint16_t address,int slot){
  if(slot<0 || slot>=4)return address;
  if(!reloading)blank_gfx[slot]=sm_seed_plm_kind(plm)!=0;
  return blank_gfx[slot]?0x9100:address; /* Original bank 89 zero-filled graphics. */
}
int sm_relic_collect(uint16_t plm){
  int kind=sm_seed_plm_kind(plm);if(!kind)return 0;
  if(kind==1){
    unsigned block=plm_block_indices[plm>>1]>>1;
    spark_x=samus_x_pos;spark_y=samus_y_pos-8;
    if(room_width_in_blocks && block<room_width_in_blocks*room_height_in_blocks){spark_x=(block%room_width_in_blocks)*16+8;spark_y=(block/room_width_in_blocks)*16+8;}
    pending_spark=1;QueueSfx1_Max6(0x37);
    pickup_count=sm_relic_count();pickup_quota=sm_relic_required();pickup_message=1;
    PlayRoomMusicTrackAfterAFrames(0x168);
    DisplayMessageBox(0x19);
    return 1;
  }
  /* SetItemBit has already persisted this location in the normal collection
   * bitmap. No equipment/reserve/ammo is awarded for the carrier PLM. */
  PlayRoomMusicTrackAfterAFrames(8);return 1;
}
void sm_relic_escape_warning(void){warning_message=1;DisplayMessageBox(0x19);}
int sm_relic_message(void){return (pickup_message || warning_message) && message_box_index==0x19;}
void sm_relic_message_closed(void){
  if(warning_message)sm_relic_escape_acknowledge();
  pickup_message=warning_message=0;
}
static void message_line(uint16_t *row,const char *s){
  s=sm_locale_text(s);int x=(32-sm_locale_length(s))/2;
  for(;*s && x<29;x++){
    int cp=sm_locale_next(&s),ch=sm_locale_base(cp);
    if(sm_locale_get()){row[x]=sm_locale_message_glyph(cp);continue;}
    if(ch>='A' && ch<='Z')row[x]=0x28e0+ch-'A';
    else if(ch>='0' && ch<='9')row[x]=0x2800+(ch-'0'+9)%10;
    else if(ch=='\'')row[x]=0x28fd;
    else if(ch==',')row[x]=0x28fb;
    else if(ch=='!')row[x]=0x28ff;
  }
}
void sm_relic_message_tiles(uint16_t *body){
  /* Same four-row body, glyphs and border geometry as a native Missile box. */
  for(int y=0;y<4;y++)for(int x=0;x<32;x++)body[y*32+x]=x<3 || x>=29?0x000e:0x284e;
  if(warning_message){
    message_line(body,"SOMETHING'S WRONG,");
    message_line(body+32,"GET TO THE SHIP !");
    message_line(body+64,"STRESS GIVES");
    message_line(body+96,"DETERMINATION !");
    return;
  }
  message_line(body,"CHOZO TABLET");
  char line[32];snprintf(line,sizeof(line),sm_locale_get()?"%d OBTENUES SUR %d":"%d COLLECTED OUT OF %d",pickup_count,pickup_quota);
  message_line(body+64,line);
}
int sm_relic_depart(void){
  if(!sm_relic_escape_active())return 0;
  if(!departing){sm_relic_escape_stop();departing=1;load_station_index=0;SaveToSram(selected_save_slot);sm_stats_finish();sm_save();}
  return 1;
}
int sm_relic_ending(void){return departing;}
static void draw_sprite(uint8_t *out,int width,int x,int y,int min_y){
  for(int py=0;py<16;py++)for(int px=0;px<16;px++){
    uint32_t c=sprite[py*16+px];int dx=x+px,dy=y+py;
    if(!(c>>24)||dx<0||dx>=width||dy<min_y||dy>=224)continue;
    uint8_t *dst=out+(dy*width+dx)*4;
    unsigned brightness=reg_INIDISP&0x80?0:reg_INIDISP&15;
    for(int channel=0;channel<3;channel++)dst[channel]=((c>>(channel*8))&255)*brightness/15;
    dst[3]=255;
  }
}
void sm_relic_icon(uint8_t *out,int width,int x,int y){draw_sprite(out,width,x,y,0);}
static void counter(uint8_t *out,int width){
  char line[40];int have=sm_relic_count(),need=sm_relic_required();
  snprintf(line,sizeof(line),sm_locale_get()?"RELIQUES %02d SUR %02d":"RELICS %02d OF %02d",have,need);
  if(have>=need)snprintf(line,sizeof(line),"RETURN TO SHIP");
  sm_native_map_text(out,width,(width-(int)strlen(line)*8)/2,32,line,0x53dec7);
}
void sm_relic_render(uint8_t *pixels){
  if(!sm_seed_active())return;
  if(game_state==8){
    /* Let the native BG3 dialog remain authoritative; emit pickup sparks when
     * gameplay resumes, so Unreal does not discard them during the freeze. */
    if(sm_message_active())return;
    if(pending_spark){sm_visual_sprite(spark_x,spark_y,140);pending_spark=0;}
    for(int k=0;k<80;k+=2)if(sm_seed_plm_kind(k)==1){
      unsigned block=plm_block_indices[k>>1]>>1;
      if(!room_width_in_blocks || block>=room_width_in_blocks*room_height_in_blocks)continue;
      unsigned data=level_data[block]&0xfff;
      if(plm_variables[k>>1]>=8 || (plm_variables[k>>1]&1))continue;
      unsigned tile=GET_WORD(RomPtr_84(0x87d5+plm_variables[k>>1]))/8;
      /* Reveal only once the native PLM draws its item block: hidden blocks
       * and unopened Chozo orbs cannot leak through this presentation. */
      if(data!=tile && data!=tile+1)continue;
      int wx=(block%room_width_in_blocks)*16,wy=(block/room_width_in_blocks)*16;
      int x=wx-layer1_x_pos,y=wy-layer1_y_pos;
      if(x< -88 || x>328 || y<32 || y>=224)continue;
      draw_sprite(pixels,256,x,y,32);draw_sprite(sm_scene_pixels,256,x,y,32);draw_sprite(sm_wide_pixels,400,x+72,y,32);
      sm_visual_sprite(wx+8,wy+8,141);
    }
  }else if(game_state==15 && sm_relic_required() && !sm_map_browser_overview()){
    counter(pixels,256);counter((uint8_t*)sm_ui_overlay(),256);counter(sm_wide_hud,400);
  }
}
