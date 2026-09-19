/* Exact background reconstruction: remove only the current OBJ texel, run the
 * reference pixel compositor, and restore every PPU field. No invented tiles. */
static uint8_t finale_clean[400*240*4],finale_scratch[256*240*4];
SM_API const uint8_t *sm_finale_background(void){return finale_clean;}
static void sm_finale_pixel(Ppu *p,int x,int line,int wx,int destination,uint8_t *out,uint8_t *meta){
 if(!sm_finale_enabled() || !sm_finale_state(55))return;
 uint8_t *clean=finale_clean+((line-1)*400+destination)*4;
 memcpy(clean,out,4);
 int owner=sm_last_sprite_owner[x];
 int sprite=(meta[0]==4 || meta[0]==6) && owner!=255;
 int palette=sprite?(p->oam[owner+1]>>9)&7:-1,wy=layer1_y_pos+line-1;
 int baby=sprite && sm_finale_state(55)<=5 && abs(wx-sm_finale_state(56))<64 && abs(wy-sm_finale_state(57))<56 && (palette==7 || palette==0);
 int samus=sprite && !baby && abs(wx-samus_x_pos)<22 && abs(wy-samus_y_pos)<38 && palette==4;
 int boss=meta[0]==1;
 // Native death switches the brain from OBJ palette 1 to palette 7. Read
 // the actual draw-hook palette rather than treating every palette-7 OBJ as baby.
 int head_palette=(Get_MotherBrain(0)->mbn_var_0D>>9)&7;
 if(sprite && !baby && !samus && palette==head_palette &&
    abs(wx-Get_MotherBrain(0x40)->base.x_pos)<48 &&
    abs(wy-Get_MotherBrain(0x40)->base.y_pos)<40)boss=1;
 // Body legs and corpse use OBJ palette 0/1 separately from the BG2 torso.
 if(sprite && !baby && !samus && (palette==0 || palette==1) &&
    wx>Get_MotherBrain(0)->base.x_pos-64 && wx<Get_MotherBrain(0)->base.x_pos+112 &&
    wy>Get_MotherBrain(0x40)->base.y_pos-40 && wy<Get_MotherBrain(0)->base.y_pos+80)boss=1;
 if(!baby && !samus && !boss)return;
 meta[1]=baby?2:samus?1:3;
 PpuZbufType saved=p->objBuffer.data[x+kPpuExtraLeftRight];
 uint8_t *target=p->renderBuffer;size_t pitch=p->renderPitch;
 Layer bg2=p->layer[1];
 if(boss){p->layer[1].mainScreenEnabled=false;p->layer[1].subScreenEnabled=false;}
 if(sprite)p->objBuffer.data[x+kPpuExtraLeftRight]=0x500;
 p->renderBuffer=finale_scratch;p->renderPitch=1024;
 int layer=sm_last_main_layer[x];
 ppu_handlePixel(p,x,line);
 memcpy(clean,finale_scratch+(line-1)*1024+x*4,4);clean[3]=255;
 p->objBuffer.data[x+kPpuExtraLeftRight]=saved;p->renderBuffer=target;p->renderPitch=pitch;
 p->layer[1]=bg2;sm_last_main_layer[x]=layer;
}
