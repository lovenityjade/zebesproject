/* Original procedural SFX: a falling digital impact and discrete stereo echoes.
 * No sampled game audio or external asset. 44.1 kHz, 1.30 s, one shot per death.
 * Called on the game thread by the same cue that disperses Samus's wireframe. */
enum { RUSH_IMPACT_DRY = 21168, RUSH_IMPACT_FRAMES = 57330 };
static int16_t rush_impact[RUSH_IMPACT_FRAMES * 2];
static int rush_impact_ready, rush_impact_fired, rush_impact_position = RUSH_IMPACT_FRAMES;

static void rush_impact_start(void) {
  if(rush_impact_fired)return;
  rush_impact_fired=1;
  if(!rush_impact_ready){
    static float dry[RUSH_IMPACT_DRY];
    uint32_t noise=0x5a454245;
    double phase=0;float low=0;
    for(int i=0;i<RUSH_IMPACT_DRY;i++){
      float t=i/44100.f;
      noise^=noise<<13;noise^=noise>>17;noise^=noise<<5;
      float white=(noise&65535)/32767.5f-1.f;
      low+=.12f*(white-low);
      phase+=6.28318530718*(65+1000*exp(-t*28))/44100;
      float attack=fminf(1.f,t/.0015f),tail=1.f-i/(float)RUSH_IMPACT_DRY;
      dry[i]=attack*tail*tail*(.40f*sinf((float)phase+1.8f*sinf(6.2831853f*173*t))*expf(-t*9)
        +.34f*low*expf(-t*7)+.15f*white*expf(-t*35)
        +.24f*sinf(6.2831853f*53*t)*expf(-t*12));
    }
    const int delay[]={0,8159,16317,25137,34839};
    const float gain[5][2]={{.72f,.72f},{.34f,.13f},{.10f,.24f},{.14f,.055f},{.035f,.08f}};
    for(int i=0;i<RUSH_IMPACT_FRAMES;i++)for(int ch=0;ch<2;ch++){
      float value=0;
      for(int tap=0;tap<5;tap++){
        int n=i-delay[tap];
        if(n>=0 && n<RUSH_IMPACT_DRY)value+=dry[n]*gain[tap][ch];
      }
      rush_impact[i*2+ch]=(int16_t)(fmaxf(-.85f,fminf(.85f,value))*32767);
    }
    rush_impact_ready=1;
  }
  rush_impact_position=0;
}
static void rush_impact_mix(int16_t *audio,int frames){
  for(int i=0;i<frames && rush_impact_position<RUSH_IMPACT_FRAMES;i++,rush_impact_position++)
    for(int ch=0;ch<2;ch++){
      int value=audio[i*2+ch]+rush_impact[rush_impact_position*2+ch];
      audio[i*2+ch]=value>INT16_MAX?INT16_MAX:value<INT16_MIN?INT16_MIN:value;
    }
}
static void rush_impact_reset(void){
  rush_impact_fired=0; rush_impact_position=RUSH_IMPACT_FRAMES;
}
