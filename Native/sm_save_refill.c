#include "sm_runs.h"
#include "sm_save_refill.h"
#include "ida_types.h"
#include "variables.h"
static int refill_enabled=1;
void sm_set_refill_before_save(int enabled){refill_enabled=enabled!=0;}
int sm_refill_before_save(void){return refill_enabled;}
void sm_refill_at_save(void){
  if(!refill_enabled || sm_run_state(0))return;
  samus_health=samus_max_health;
  samus_reserve_health=samus_max_reserve_health;
  samus_missiles=samus_max_missiles;
  samus_super_missiles=samus_max_super_missiles;
  samus_power_bombs=samus_max_power_bombs;
}
