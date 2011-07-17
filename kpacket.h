#ifndef __KPACKET_H__
#define __KPACKET_H__
typedef struct kspacket{
  s64 accumulated_time;
  s64 in_time;
  s64 rtt;
  uint32_t id;
  uint8_t fails;
  uint8_t retries;
  uint8_t samples[3];
} packet_t;
#endif
