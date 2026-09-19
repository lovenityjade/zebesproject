/* Reconstruct reviewed decoration atlases from the player's validated ROM.
 * Recipes contain tile references and authored XOR edits, never raster payloads. */
#include "sm_rtl.h"
#include "funcs.h"
static int decor_word(const uint8_t *p){return p[0]|p[1]<<8;}
static uint32_t decor_long(const uint8_t *p){return p[0]|p[1]<<8|p[2]<<16;}
static int decor_rom_image(FILE *f,struct DecorImage *image){
 unsigned w,h,count;uint8_t *data=NULL,*defs=NULL,*gfx=NULL,*pal=NULL;int valid=0,last=-1;
 if(fscanf(f,"%u %u %u",&w,&h,&count)!=3||!w||!h||w>512||h>512||w%16||h%16||count>w*h/256)return 0;
 data=calloc(w*h,4);defs=calloc(65536,1);gfx=calloc(65536,1);pal=calloc(65536,1);
 if(!data||!defs||!gfx||!pal)goto done;
 for(unsigned n=0;n<count;n++){
  unsigned dst,set,tile;
  if(fscanf(f,"%u %u %u",&dst,&set,&tile)!=3||dst>=w*h/256||set>28||tile>=1024)goto done;
  if((int)set!=last){
   uint32_t address=0x8f0000|decor_word(RomFixedPtr(0x8fe7a7)+set*2);
   const uint8_t *entry=RomFixedPtr(address);
   memset(defs,0,65536);memset(gfx,0,65536);memset(pal,0,65536);
   DecompressToMem(0xb9a09d,defs);DecompressToMem(decor_long(entry),defs+2048);
   DecompressToMem(0xb98000,gfx+0x5000);DecompressToMem(decor_long(entry+3),gfx);
   DecompressToMem(decor_long(entry+6),pal);last=set;
  }
  for(int part=0;part<4;part++){
   unsigned entry=decor_word(defs+tile*8+part*2),base=(entry&1023)*32,palette=((entry>>10)&7)*16;
   for(int y=0;y<8;y++)for(int x=0;x<8;x++){
    int row=entry&0x8000?7-y:y,bit=entry&0x4000?x:7-x,ci=0;
    for(int plane=0;plane<4;plane++)ci|=((gfx[base+row*2+(plane&1)+(plane>=2?16:0)]>>bit)&1)<<plane;
    if(!ci)continue;
    unsigned color=decor_word(pal+(palette+ci)*2);
    uint8_t *p=data+(((dst/(w/16))*16+(part/2)*8+y)*w+(dst%(w/16))*16+(part%2)*8+x)*4;
    for(int c=0;c<3;c++){int v=(color>>((2-c)*5))&31;p[c]=(v<<3)|(v>>2);}p[3]=255;
   }
  }
 }
 unsigned edits;if(fscanf(f,"%u",&edits)!=1||edits>w*h*4)goto done;
 for(unsigned n=0;n<edits;n++){unsigned index,value;if(fscanf(f,"%u %u",&index,&value)!=2||index>=w*h*4||value>255)goto done;data[index]^=value;}
 int ch;do{ch=fgetc(f);}while(ch==' '||ch=='\n'||ch=='\r'||ch=='\t');if(ch!=EOF)goto done;
 image->data=data;image->w=w;image->h=h;data=NULL;valid=1;
 done:free(data);free(defs);free(gfx);free(pal);return valid;
}
