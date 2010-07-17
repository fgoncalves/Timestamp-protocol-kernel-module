#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include "timestamp.h"
#include "macros.h"
#include "statistics.h"

char file[50] = {0};
FILE* file_fd;

void init_statistics(){
  sprintf(file, "ostatistics-%u.out",getpid());

  file_fd = fopen(file, "w");

  if(file_fd == null){
    print_error("Failed to open statistics file");
    exit(EXIT_ERROR);
  }

  fprintf(file_fd,"# Timestamp\t\t\tPacket ID\t\t\tSource\n");
}

void write_data(unsigned long long tstamp, unsigned int id, char* source){
  struct timespec ts = ns_to_timespec(tstamp);
  struct tm* info;
  char buffer[80];
  unsigned long long milli_seconds;

  info = localtime(&ts.tv_sec);
  strftime(buffer,80, "%X",info);
  milli_seconds = (unsigned long long) ts.tv_nsec / (unsigned long long) 1E6;
  //  fprintf(file_fd,"%s:%llu\t\t\t%u\t\t\t%s\n",buffer,milli_seconds, id, source);
  fprintf(file_fd,"%llu\t\t\t%u\t\t\t%s\n",tstamp, id, source);
}

void close_statistics(){
  fflush(file_fd);
  fclose(file_fd);
}

char* get_statistics_file(){
  return file;
}
