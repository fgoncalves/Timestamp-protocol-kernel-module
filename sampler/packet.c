#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <stdint.h>
#include "packet.h"
#include "timestamp.h"
#include "gps_time.h"
#include "macros.h"

unsigned int samples_per_second = 25;
unsigned char emulate_ts = false;
uint8_t gather_gps_time = false;

static int64_t htonll(int64_t s){
  char* ss = (char*) &s;
  char v[8] = {0};
  memcpy(v, &s, sizeof(int64_t));
  ss[0] = v[7];
  ss[1] = v[6];
  ss[2] = v[5];
  ss[3] = v[4];
  ss[4] = v[3];
  ss[5] = v[2];
  ss[6] = v[1];
  ss[7] = v[0];
  return *(int64_t*) ss;
}

void swap_packet_byte_order(packet_t* packet){
  packet->accumulated_time = htonll(packet->accumulated_time);  
  packet->in_time = htonll(packet->in_time);  
  packet->rtt = htonll(packet->rtt);  
  packet->gps_time = htonll(packet->gps_time);  
  packet->id = htonl(packet->id);
}

void init_packet(packet_t* packet){
  static unsigned int id = 0;
  static s64 time = 0;
  struct timespec ts;
  s64 creation_timestamp;

  packet->rtt = 0;
  packet->id = id;
  packet->accumulated_time = 0;

  if(emulate_ts){
    if( ! time){
      clock_gettime(CLOCK_REALTIME, &ts);
      creation_timestamp = timespec_to_ns(ts);
      time = timespec_to_ns(ts);
      memcpy(& (packet->in_time), & creation_timestamp, sizeof(s64));
    }else{
      time += (1000000000 / samples_per_second);
      memcpy(& (packet->in_time), & time, sizeof(s64));
    }
  }else{
    clock_gettime(CLOCK_REALTIME, &ts);
    creation_timestamp = timespec_to_ns(ts);
    memcpy(& (packet->in_time), & creation_timestamp, sizeof(s64));
  }
  id++;

  if(gather_gps_time){
    struct timeval tv;
    memset(&tv, 0, sizeof(struct timeval));    
    getgpstimeofday(&tv, NULL);
    time = timeval_to_ns(tv);
    memcpy(& (packet->gps_time), & time, sizeof(s64));
  }
}
