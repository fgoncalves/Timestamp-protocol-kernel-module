#ifndef __PACKET_H__
#define __PACKET_H__

#include <stdint.h>
#include "timestamp.h"

extern unsigned int samples_per_second;
extern unsigned char emulate_ts;

typedef struct {
  s64 accumulated_time;
  s64 in_time;
  unsigned int id;
  unsigned char samples[3];
} packet_t;

extern void init_packet(packet_t* packet);
extern void swap_packet_byte_order(packet_t* packet);
#endif
