#ifndef __PACKET_H__
#define __PACKET_H__

#include <stdint.h>
#include "timestamp.h"
#include "../kpacket.h"

extern unsigned int samples_per_second;
extern unsigned char emulate_ts;
extern uint8_t gather_gps_time;

extern void init_packet(struct kspacket* packet);
extern void swap_packet_byte_order(struct kspacket* packet);
#endif
