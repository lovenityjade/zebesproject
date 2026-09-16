/* Shared original-level decoding and VARIA-edit reconstruction.
 * Include after the domain-specific sm_*_rom_assets.inc in each module. */
static void sm_room_rom_load(uint8_t *target,size_t size){
  uint8_t level[65536],packed[32768];
  memset(target,0,size);
  for(unsigned i=0;i<sizeof(world_record_copies)/sizeof(*world_record_copies);i++)
    memcpy(target+world_record_copies[i].destination,
           world_record_bytes+world_record_copies[i].source,world_record_copies[i].size);
  for(unsigned i=0;i<sizeof(world_level_programs)/sizeof(*world_level_programs);i++){
    memset(level,0,sizeof(level));
    DecompressToMem(world_level_programs[i].source,level);
    for(unsigned e=world_level_programs[i].edit_first,end=e+world_level_programs[i].edit_count;e<end;e++)
      level[world_level_edits[e].offset]=world_level_edits[e].value;
    const uint8_t *command=world_level_commands+world_level_programs[i].command_first;
    unsigned input=0,output=0;
    for(;;){
      unsigned first=*command++;packed[output++]=first;if(first==255)break;
      unsigned mode=first>>5,length=(first&31)+1;
      if(mode==7){unsigned next=*command++;packed[output++]=next;mode=(first>>2)&7;length=((first&3)<<8|next)+1;}
      if(mode==0){memcpy(packed+output,level+input,length);output+=length;}
      else if(mode==1 || mode==3)packed[output++]=level[input];
      else if(mode==2){packed[output++]=level[input];packed[output++]=level[input+1];}
      else{unsigned count=mode>=6?1:2;memcpy(packed+output,command,count);output+=count;command+=count;}
      input+=length;
    }
    for(unsigned c=0;c<sizeof(world_level_copies)/sizeof(*world_level_copies);c++)if(world_level_copies[c].program==i){
      uint8_t *destination=target+world_level_copies[c].destination;
      if(world_level_copies[c].count)memcpy(destination,packed+world_level_copies[c].source,world_level_copies[c].count);
      memset(destination+world_level_copies[c].count,255,world_level_copies[c].total-world_level_copies[c].count);
    }
  }
}
