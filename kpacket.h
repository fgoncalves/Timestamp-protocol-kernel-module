#ifndef __PACKET_H__
#define __PACKET_H__
typedef struct {
  s64 accumulated_time;
  s64 in_time;
  s64 rtt;
  uint32_t id;
  uint8_t samples[3];
} packet_t;
#endif
