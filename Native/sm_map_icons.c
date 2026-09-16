#include "sm_map_icons.h"
#include "sm_areas.h"
#include "sm_minimizer.h"
#include "sm_connections.h"
#include "sm_doors.h"
#include "sm_seed.h"
#include "sm_map_browser.h"
#include "sm_objectives.h"
#include "sm_objective_events.h"
#include "ida_types.h"
#include "variables.h"
#include "sm_rtl.h"
#include "sm_cpu_infra.h"
#include "sm_map_icons.inc"
#include "sm_objective_map.inc"

extern uint8_t sm_wide_hud[];
static int explored(const MapIconPosition *p){
  if(p->area>=6)return 0;
  const uint8_t *bits=p->area==area_index?map_tiles_explored:(const uint8_t*)explored_map_tiles_saved+p->area*256;
  return (bits[p->byte]&p->mask)!=0;
}
static int portal_sprite(const MapPortal *portals,int count,int index,int destination){
  if(index<0 || index>=count || destination<0 || destination>=count)return -1;
  const MapIconPosition *p=&portals[index].display;
  if(!explored(p))return sm_map_fully_known() || map_station_byte_array[p->area]?MAP_UNKNOWN_PORTAL:-1;
  /* VARIA reveals the destination only after both ends are explored. An
   * explored source with an unexplored target intentionally has no icon. */
  return explored(&portals[destination].actual)?portals[destination].sprite:-1;
}
static void draw_at(uint8_t *out,int width,int area_id,int map_x,int map_y,int offset_x,int offset_y,const uint16_t *pixels){
  int pause=game_state==15,area=pause?sm_map_browser_view_area():area_index;
  if(area!=area_id)return;
  int left,right,top,bottom,x,y;
  if(pause){
    left=8;right=width-8;top=48;bottom=192;
    x=map_x-(int16_t)reg_BG1HOFS+(width-256)/2;y=map_y-(int16_t)reg_BG1VOFS;
  }else{
    int mx=room_x_coordinate_on_map+(samus_x_pos>>8),my=room_y_coordinate_on_map+(samus_y_pos>>8)+1;
    /* Preserve Samus's native minimap cursor on the occupied tile. */
    if(map_x==mx*8 && map_y==my*8)return;
    left=width==400?336:208;right=left+(width==400?7:5)*8;top=0;bottom=(width==400?4:3)*8;
    x=left+map_x+(-mx+(width==400?4:2))*8;y=map_y+(-my+1)*8;
  }
  x+=offset_x;y+=offset_y;
  const uint8_t *brightness=g_snes->ppu->brightnessMult;
  for(int dy=0;dy<8;dy++)for(int dx=0;dx<8;dx++){
    uint16_t c=pixels[dy*8+dx];int px=x+dx,py=y+dy;
    if(c==65535 || px<left || px>=right || py<top || py>=bottom)continue;
    uint8_t *pixel=out+(py*width+px)*4;
    pixel[0]=brightness[(c>>10)&31];pixel[1]=brightness[(c>>5)&31];pixel[2]=brightness[c&31];pixel[3]=255;
  }
}
static void draw(uint8_t *out,int width,const MapIconPosition *p,int sprite){
  if(sprite<0 || sprite>=sizeof(map_icon_sprites)/sizeof(*map_icon_sprites))return;
  draw_at(out,width,p->area,p->x*8,p->y*8,map_icon_sprites[sprite].x,map_icon_sprites[sprite].y,map_icon_sprites[sprite].pixels);
}
void sm_objective_map_render(uint8_t *pixels){
  if(!g_snes || g_snes->ppu->forcedBlank || game_state!=15 || pause_screen_mode ||
     sm_map_browser_overview() || !sm_objectives_state(7))return;
  int ranks[OBJECTIVE_MAP_SPOTS],points[OBJECTIVE_MAP_SPOTS];
  for(int s=0;s<OBJECTIVE_MAP_SPOTS;s++)ranks[s]=-1;
  for(int rank=0;rank<sm_objectives_state(0);rank++){
    int goal=sm_objectives_goal(rank);if(goal<0 || goal>=59)continue;
    int count=objective_map_goals[goal].count,category=objective_map_goals[goal].category;
    for(int j=0;j<count;j++){
      int point=objective_map_members[objective_map_goals[goal].offset+j],spot=objective_map_points[point].spot;
      if(sm_minimizer_active() && objective_map_points[point].region!=255 && !sm_minimizer_region(-1,objective_map_points[point].region))continue;
      int old=ranks[spot]<0?-1:sm_objectives_goal(ranks[spot]);
      /* Match writeObjectivesMapIcons at generation, including conflicts
       * with completed goals. Completion does not select another winner. */
      if(old<0 || (objective_map_goals[old].category==category && objective_map_goals[old].count>count) ||
         (objective_map_goals[old].category!=category && category==OBJECTIVE_MAP_MEMES)){
        ranks[spot]=rank;points[spot]=point;
      }
    }
  }
  for(int s=0;s<OBJECTIVE_MAP_SPOTS;s++){
    int rank=ranks[s];if(rank<0 || sm_objective_event(162+2*rank))continue;
    int point=points[s],event=objective_map_points[point].event;
    if(!event || sm_objective_event(event))continue;
    int area=objective_map_points[point].area,x=objective_map_points[point].x,y=objective_map_points[point].y;
    draw_at(pixels,256,area,x,y,1,-1,objective_map_sprites[rank]);
    draw_at((uint8_t*)sm_ui_overlay(),256,area,x,y,1,-1,objective_map_sprites[rank]);
    draw_at(sm_wide_hud,400,area,x,y,1,-1,objective_map_sprites[rank]);
  }
}
static void render(uint8_t *out,int width){
  for(unsigned i=0;i<sizeof(map_door_positions)/sizeof(*map_door_positions);i++){
    const MapIconPosition *p=&map_door_positions[i].position;
    unsigned bit=map_door_positions[i].opened_bit;
    int color=sm_doors_map_color(map_door_positions[i].id);
    if(color<=0 || color>=9 || !explored(p) || (opened_door_bit_array[bit>>3]&(1<<(bit&7))))continue;
    draw(out,width,p,map_door_sprites[color][map_door_positions[i].facing]);
  }
  if(sm_minimizer_active()){
    for(int i=0;i<40;i++){
      int j=sm_minimizer_destination(i);
      const MapPortal *src=i<32?&map_area_portals[i]:&map_boss_portals[i-32];
      const MapPortal *dst=j<32?&map_area_portals[j]:&map_boss_portals[j-32];
      int sprite=explored(&src->display)?(explored(&dst->actual)?dst->sprite:-1):
        (sm_map_fully_known() || map_station_byte_array[src->display.area]?MAP_UNKNOWN_PORTAL:-1);
      draw(out,width,&src->display,sprite);
    }
    return;
  }
  for(int i=0;i<32;i++)draw(out,width,&map_area_portals[i].display,
      portal_sprite(map_area_portals,32,i,sm_areas_destination(i)));
  for(int i=0;i<8;i++)draw(out,width,&map_boss_portals[i].display,
      portal_sprite(map_boss_portals,8,i,sm_connections_destination(i)));
}
void sm_map_icons_render(uint8_t *pixels){
  if(!sm_seed_active() || !g_snes || g_snes->ppu->forcedBlank ||
     (game_state!=8 && game_state!=15) || (game_state==15 && pause_screen_mode) || sm_map_browser_overview())return;
  render(pixels,256);render((uint8_t*)sm_ui_overlay(),256);render(sm_wide_hud,400);
}
