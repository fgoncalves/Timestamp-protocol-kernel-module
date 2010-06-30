#include "timestamp.h"

#define nsec_per_sec    1000000000L

static unsigned long div_ulong_rem(unsigned long long dividend, unsigned int divisor, unsigned int *remainder){
  *remainder = dividend % divisor;
  return dividend / divisor;
}

struct timespec ns_to_timespec(unsigned long long nsec){
  struct timespec ts;
  unsigned int rem;
  
  if (!nsec)
    return (struct timespec) {0, 0};
  
  ts.tv_sec = div_ulong_rem(nsec, nsec_per_sec, &rem);
  ts.tv_nsec = rem;
  
  return ts;
}

unsigned long long timespec_to_ns(const struct timespec ts){
  return ((unsigned long long) ts.tv_sec * (unsigned long long) nsec_per_sec) + (unsigned long long) ts.tv_nsec;
}
