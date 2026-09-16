#pragma once
#include "sm_bridge.h"
/* Immutable eight-AP boss domain; ordinary/area connections remain unchanged. */
SM_API const char *sm_connections_catalog_sha256(void);
SM_API int sm_connections_configure(int slot,const uint8_t *destinations,int count,const char *catalog);
void sm_connections_reset(void);
void sm_connections_activate(int slot);
void sm_connections_capture(uint8_t *rom);
void sm_connections_apply(uint8_t *rom,int randomized);
int sm_connections_door(void);
int sm_connections_destination(int index);
void sm_connections_arrival(uint16_t asm_ptr,int incompatible,uint16_t x,uint16_t y,int exit_fix);
void sm_connections_room(void);
uint16_t sm_connections_spark_health(void);
int sm_connections_test_destination(int index,int *room,int *door,int *x,int *y);
void sm_connections_restore(uint8_t *rom);
