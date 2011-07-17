#ifndef __STATISTICS_H__
#define __STATISTICS_H__

#include "timestamp.h"

extern void write_data(s64 tstamp, s64 delay, s64 air_time, unsigned int id, char* source, unsigned char* collected_data, uint8_t retries, uint8_t fails);
extern void close_statistics();
extern void init_statistics();
extern char* get_statistics_file();
#endif
