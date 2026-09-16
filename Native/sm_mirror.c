#include "sm_mirror.h"

/* Mirror was explicitly excluded from the release. Preserve the ABI and native
 * call sites so ordinary saves remain compatible, but reject activation instead
 * of embedding an unused alternate world's ROM-derived graphics/level payload. */
int sm_mirror_configure(int slot,int enabled,const char *catalog){
  (void)catalog;
  return slot>=0 && slot<4 && enabled==0;
}
void sm_mirror_reset(void){}
void sm_mirror_activate(int slot){(void)slot;}
void sm_mirror_apply(int randomized){(void)randomized;}
int sm_mirror_active(void){return 0;}
int sm_mirror_door(uint32_t address){(void)address;return 0;}
void sm_mirror_capture(uint8_t *rom){(void)rom;}
void sm_mirror_restore(uint8_t *rom){(void)rom;}
void sm_mirror_apply_rom(uint8_t *rom){(void)rom;}
uint32_t sm_mirror_item_address(int index,uint32_t canonical){
  (void)index;return canonical;
}
