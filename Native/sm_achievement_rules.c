#include "sm_achievements.h"
uint64_t sm_achievements_evaluate(const SmAchievementFacts *n,const SmAchievementFacts *p){
 if(!n||!n->valid)return 0;
 uint64_t bits=0;int mode=n->randomized?25:5;
 #define AWARD(i,condition) do{if(condition)bits|=UINT64_C(1)<<(mode+(i));}while(0)
 if(!n->randomized){
  AWARD(0,n->items&4);AWARD(1,n->items&0x1000);AWARD(2,n->items&1);AWARD(3,n->items&0x20);
  AWARD(4,n->items&0x2000);AWARD(5,n->items&0x200);AWARD(6,n->items&8);
  AWARD(7,(n->beams&0x100f)==0x100f);AWARD(8,n->health>=1099);AWARD(9,n->missiles>=100);
  for(int i=0;i<4;i++)AWARD(10+i,n->bosses&(1<<i));
  AWARD(14,(n->bosses&15)==15);AWARD(15,(n->bosses&0xf0)==0xf0);
  AWARD(16,n->checks>=100);AWARD(17,n->animals);AWARD(18,n->finished);
  AWARD(19,n->finished&&n->frames>0&&n->frames<3*60*60*60);
 }else{
  AWARD(0,n->checks>=1);AWARD(1,n->checks>=10);AWARD(2,n->checks>=25);AWARD(3,n->checks>=50);
  AWARD(4,n->total>0&&n->checks>=n->total);
  for(int i=0;i<4;i++)AWARD(5+i,n->bosses&(1<<i));
  AWARD(9,(n->bosses&15)==15);AWARD(10,(n->items&0x21)==0x21);AWARD(11,(n->beams&0x100f)==0x100f);
  AWARD(12,n->required>0&&n->tablets>0);AWARD(13,n->required>0&&n->tablets>=(n->required+1)/2);
  AWARD(14,n->required>0&&n->tablets>=n->required);AWARD(15,n->finished);
  AWARD(16,n->finished&&n->required>0&&n->tablets>=n->required);
  const int same=p&&p->valid&&p->randomized&&p->session==n->session&&p->slot==n->slot;
  AWARD(17,same&&!(p->items&0x20)&&(n->items&0x20)&&!(n->items&1));
  AWARD(18,same&&!(p->bosses&1)&&(n->bosses&1)&&!(n->items&4));
  AWARD(19,n->finished&&n->frames>0&&n->frames<2*60*60*60);
 }
 #undef AWARD
 return bits;
}
