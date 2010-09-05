#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdint.h>
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

  fprintf(file_fd,"# Timestamp\t\t\tDelay\t\t\tPacket ID\t\t\tSource\t\t\tData Value\n");
}

void write_data(unsigned long long tstamp, unsigned long long delay, unsigned int id, char* source, unsigned char* collected_data){
  struct timespec ts = ns_to_timespec(tstamp);
  struct tm* info;
  char buffer[80];
  unsigned long long milli_seconds;
  int32_t collected_data_in_integer_format = 0;

  info = localtime(&ts.tv_sec);
  strftime(buffer,80, "%X",info);
  milli_seconds = (unsigned long long) ts.tv_nsec / (unsigned long long) 1E6;
  memcpy(&collected_data_in_integer_format, &collected_data, 3);
  fprintf(file_fd,"%s:%llu\t\t\t%llu\t\t\t%u\t\t\t%s\t\t%d\n",buffer,milli_seconds, delay, id, source, collected_data_in_integer_format);
  fprintf(file_fd,"%llu\t\t\t%llu\t\t\t%u\t\t\t%s\t\t%d\n",tstamp,delay, id, source, collected_data_in_integer_format);
}

void close_statistics(){
  fflush(file_fd);
  fclose(file_fd);
}

char* get_statistics_file(){
  return file;
}
