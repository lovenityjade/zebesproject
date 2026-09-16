/* Synthetic two-layer fixture: proves rendering invariants, not room support. */
#include "../Native/sm_scene.c"
bool g_new_ppu=false;
uint8 g_ram[0x20000];
Snes *g_snes;
int main(void) {
  static Snes snes;static Ppu p;
  static uint8_t original[256*240*4],before[256*240*4];
  g_snes=&snes;snes.ppu=&p;p.snes=&snes;
  ppu_reset(&p);PpuBeginDrawing(&p,original,1024,0);
  p.mode=1;p.brightness=15;p.forcedBlank=false;
  p.layer[0].mainScreenEnabled=p.layer[1].mainScreenEnabled=true;
  p.bgLayer[0].tilemapAdr=0x1000;p.bgLayer[1].tilemapAdr=0x1400;
  p.cgram[1]=31;p.cgram[2]=31<<5;p.cgram[3]=31<<10;
  // Solid BG1 panel on left; alternating BG2 stripes behind it.
  for(int row=0;row<8;++row) { p.vram[16+row]=0x00ff;p.vram[32+row]=0xff00;p.vram[48+row]=0xffff; }
  for(int y=0;y<32;++y)for(int x=0;x<32;++x) {
    p.vram[0x1000+y*32+x]=x<8?0x2001:0;
    p.vram[0x1400+y*32+x]=(x&1)?2:3;
  }
  sm_scene_active=1;sm_scene_parallax=0;
  for(int line=0;line<=224;++line)ppu_runLine(&p,line);
  memcpy(before,original,sizeof(before));
  sm_scene_parallax=1;sm_scene_camera_x=8;sm_scene_camera_y=0;
  for(int line=0;line<=224;++line)ppu_runLine(&p,line);
  assert(!memcmp(before,original,sizeof(before)));
  assert(p.bgLayer[1].hScroll==0 && p.bgLayer[1].vScroll==0);
  int changed=0;
  for(int y=0;y<224;++y)for(int x=0;x<256;++x) {
    int different=memcmp(original+(y*256+x)*4,sm_scene_pixels+(y*256+x)*4,3)!=0;
    if(x<64||y<32)assert(!different);else changed+=different;
  }
  assert(changed>10000);
  // Unknown bright graphics never become emitters.
  for(int i=0;i<256*224;++i)assert(!(sm_scene_emission[i*4]|sm_scene_emission[i*4+1]|sm_scene_emission[i*4+2]));
  sm_scene_finish();
  for(int i=0;i<64*60*4;++i)assert(sm_scene_lights[i]==0);
  projectile_bomb_instruction_ptr[0]=1;projectile_x_pos[0]=160;projectile_y_pos[0]=120;
  sm_scene_finish();assert(sm_scene_lights[(30*64+40)*4+2]>100);
  projectile_bomb_instruction_ptr[0]=0;sm_scene_finish();
  for(int i=0;i<64*60*4;++i)assert(sm_scene_lights[i]==0);
  fx_type=2;lava_acid_y_pos=180;sm_scene_finish();
  assert(sm_scene_lights[(45*64+40)*4+2]>100);
  // BG3 rain replacement must not touch the reference, HUD or PPU registers.
  fx_type=10;game_state=8;gameplay_BG3SC=0x5c;
  p.bg3priority=true;p.layer[2].mainScreenEnabled=true;
  p.bgLayer[2].tilemapAdr=0x1800;p.bgLayer[2].tileAdr=0x2000;
  for(int y=0;y<32;++y)for(int x=0;x<32;++x)p.vram[0x1800+y*32+x]=0x2001;
  for(int row=0;row<8;++row)p.vram[0x2008+row]=0xffff;
  sm_scene_parallax=0;sm_scene_weather=0;
  for(int line=0;line<=224;++line)ppu_runLine(&p,line);
  memcpy(before,original,sizeof before);
  sm_scene_weather=1;
  for(int line=0;line<=224;++line)ppu_runLine(&p,line);
  assert(!memcmp(before,original,sizeof before));
  assert(!memcmp(original,sm_scene_pixels,32*1024));
  assert(memcmp(original+32*1024,sm_scene_pixels+32*1024,192*1024));
  assert(p.layer[2].mainScreenEnabled && !p.layer[2].subScreenEnabled);
  // A message box owns BG3: never suppress it, even with weather enabled.
  gameplay_BG3SC=0x58;
  for(int line=0;line<=224;++line)ppu_runLine(&p,line);
  assert(!memcmp(original,sm_scene_pixels,224*1024));
  sm_scene_weather=0;
  ppu_reset(&p);memset(g_ram,0,sizeof(g_ram));
  game_state=8;room_ptr=0xdf45;p.mode=7;p.brightness=15;p.forcedBlank=false;
  p.layer[0].mainScreenEnabled=true;p.m7matrix[0]=p.m7matrix[3]=256;
  p.mathEnabled[0]=true;
  for(int j=0;j<16384;++j)p.vram[j]=0xa5;
  for(int j=0;j<64;++j)p.vram[0xa5*64+j]|=checker_pattern[j]<<8;
  p.cgram[72]=12<<10;
  static uint16_t vram_before[32768];memcpy(vram_before,p.vram,sizeof(vram_before));
  sm_scene_parallax=0;
  for(int line=0;line<=224;++line)ppu_runLine(&p,line);
  memcpy(before,original,sizeof(before));
  sm_scene_parallax=1;sm_scene_camera_x=2;
  for(int line=0;line<=224;++line)ppu_runLine(&p,line);
  assert(!memcmp(vram_before,p.vram,sizeof(vram_before)));
  assert(!memcmp(before,original,sizeof(before)));
  assert(memcmp(original+32*1024,sm_scene_pixels+32*1024,192*1024));
  printf("PASS: BG2 changed %d pixels; original, foreground, HUD, registers unchanged; unreviewed bright tiles non-emissive.\n",changed);
  return 0;
}
