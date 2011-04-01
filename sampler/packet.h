#ifndef __PACKET_H__
#define __PACKET_H__

#include <stdint.h>
#include "../kpacket.h"
#include "timestamp.h"

extern unsigned int samples_per_second;
extern unsigned char emulate_ts;

extern void init_packet(packet_t* packet);
extern void swap_packet_byte_order(packet_t* packet);
#endif
