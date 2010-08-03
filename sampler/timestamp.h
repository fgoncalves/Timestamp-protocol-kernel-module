#ifndef __TIMESTAMP_H__
#define __TIMESTAMP_H__

#include <time.h>
extern struct timespec ns_to_timespec(unsigned long long nsec);
extern unsigned long long timespec_to_ns(const struct timespec ts);
#endif
