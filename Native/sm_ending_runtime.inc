/* Appended to an isolated copy of upstream sm_rtl.c. No changes to upstream.
 * Keep the renderer, RAM, SRAM, DSP and queued music commands in memory;
 * preview frames deliberately bypass the gameplay input recorder. */
static ByteArray ending_snapshot;
static void ending_state_io(SaveLoadFunc *io,void *ctx) {
  snes_saveload(g_snes,io,ctx);
  io(ctx,g_ram,sizeof(g_ram));
  io(ctx,g_snes->ppu,sizeof(*g_snes->ppu));
  io(ctx,g_snes->input1,sizeof(*g_snes->input1));
  io(ctx,g_snes->input2,sizeof(*g_snes->input2));
  io(ctx,g_spc_player,sizeof(*g_spc_player));
  io(ctx,g_spc_player->dsp,sizeof(*g_spc_player->dsp));
  io(ctx,&snes_frame_counter,sizeof(snes_frame_counter));
  io(ctx,&g_apu_write,sizeof(g_apu_write));
  io(ctx,g_apu_write_ents,sizeof(g_apu_write_ents));
  io(ctx,&g_apu_write_ent_pos,sizeof(g_apu_write_ent_pos));
  io(ctx,&g_apu_queue_size,sizeof(g_apu_queue_size));
  io(ctx,&g_apu_time_since_empty,sizeof(g_apu_time_since_empty));
}
void sm_ending_runtime_capture(void) {
  ByteArray_Destroy(&ending_snapshot);
  memset(&ending_snapshot,0,sizeof(ending_snapshot));
  ending_state_io(saveFunc,&ending_snapshot);
}
void sm_ending_runtime_restore(void) {
  LoadFuncState state={ending_snapshot.data,ending_snapshot.data+ending_snapshot.size};
  ending_state_io(loadFunc,&state);
  ByteArray_Destroy(&ending_snapshot);
  memset(&ending_snapshot,0,sizeof(ending_snapshot));
}
void sm_ending_runtime_frame(void) {
  g_rtl_runframe(0,0);
  ++snes_frame_counter;
  RtlPushApuState();
}

/* Boss Rush retains an independent origin snapshot across all ten arenas. */
static ByteArray rush_snapshot;
static uint8_t rush_rom[3145728];
void sm_rush_snapshot(int operation) {
  if(operation==0){
    ByteArray_Destroy(&rush_snapshot);memset(&rush_snapshot,0,sizeof(rush_snapshot));
    ending_state_io(saveFunc,&rush_snapshot);
    memcpy(rush_rom,g_rom,sizeof(rush_rom));
  }else if(operation==1 && rush_snapshot.size){
    LoadFuncState state={rush_snapshot.data,rush_snapshot.data+rush_snapshot.size};
    ending_state_io(loadFunc,&state);
    memcpy((void*)g_rom,rush_rom,sizeof(rush_rom));
  }else if(operation==2){
    ByteArray_Destroy(&rush_snapshot);memset(&rush_snapshot,0,sizeof(rush_snapshot));
  }
}
