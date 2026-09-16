#include "sm_randomizer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int main(int argc,char **argv) {
  if(argc!=3 && argc!=4)return 2;
  char *buffer=calloc(2097152,1);
  if(!buffer)return 1;
  int (*service)(const char*,const char*,char*,int)=sm_randomizer_generate;
  if(argc==4){
    if(!strcmp(argv[3],"validate"))service=sm_randomizer_validate;
    else if(!strcmp(argv[3],"tracker"))service=sm_tracker_evaluate;
    else return 2;
  }
  int n=service(argv[1],argv[2],buffer,2097152);
  if(n<=0||n>2097152){fprintf(stderr,"%s\n",sm_randomizer_error());return 1;}
  puts(buffer);free(buffer);return 0;
}
