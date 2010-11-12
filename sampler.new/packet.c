#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "packet.h"
#include "timestamp.h"
#include "macros.h"

unsigned int samples_per_second = 25;
unsigned char emulate_ts = false;

//TODO: Transform this into a cycle
void swap_packet_byte_order(packet_t* packet){
  unsigned char* bytes = (unsigned char*) packet;

  uint32_t word;

  //first half of accumulated_time
  memcpy(&word, bytes, 4);
  word = htonl(word);
  memcpy(bytes, &word, 4);

  //second half of accumulated_time
  memcpy(&word, bytes + 4, 4);
  word = htonl(word);
  memcpy(bytes + 4, &word, 4);

  //first half of in_time
  memcpy(&word, bytes + 8, 4);
  word = htonl(word);
  memcpy(bytes + 8, &word, 4);

  //second half of in_time
  memcpy(&word, bytes + 12, 4);
  word = htonl(word);
  memcpy(bytes + 12, &word, 4);

  //first half of avg_rtt
  memcpy(&word, bytes + 16, 4);
  word = htonl(word);
  memcpy(bytes + 16, &word, 4);

  //second half of avg_rtt
  memcpy(&word, bytes + 20, 4);
  word = htonl(word);
  memcpy(bytes + 20, &word, 4);

  //first half of avg_rtt
  memcpy(&word, bytes + 24, 4);
  word = htonl(word);
  memcpy(bytes + 24, &word, 4);

  //second half of avg_rtt
  memcpy(&word, bytes + 28, 4);
  word = htonl(word);
  memcpy(bytes + 28, &word, 4);

  //id
  memcpy(&word, bytes + 32, 4);
  word = htonl(word);
  memcpy(bytes + 32, &word, 4);

  packet = (packet_t*) bytes;
  //TODO: samples need conversion?
}

void init_packet(packet_t* packet){
  static unsigned int id = 0;
  static s64 time = 0;
  struct timespec ts;
  s64 creation_timestamp;

  packet->avg_rtt = 0;
  packet->rtt_variance = 0;
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
    time = timespec_to_ns(ts);
    memcpy(& (packet->in_time), & creation_timestamp, sizeof(s64));
  }
  id++;
}
