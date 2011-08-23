#include "timestamp.h"

#define nsec_per_sec    1000000000L
#define nsec_per_usec   1000L

static long div_ulong_rem(s64 dividend, int divisor, int *remainder){
  *remainder = dividend % divisor;
  return dividend / divisor;
}

struct timespec ns_to_timespec(s64 nsec){
  struct timespec ts;
  int rem;
  
  if (!nsec)
    return (struct timespec) {0, 0};
  
  ts.tv_sec = div_ulong_rem(nsec, nsec_per_sec, &rem);
  ts.tv_nsec = rem;
  
  return ts;
}

s64 timespec_to_ns(const struct timespec ts){
  return ((s64) ts.tv_sec * (s64) nsec_per_sec) + (s64) ts.tv_nsec;
}

s64 timeval_to_ns(const struct timeval tv){
  return ((s64) tv.tv_sec * (s64) nsec_per_sec) + ((s64) tv.tv_usec * (s64) nsec_per_usec);
}
