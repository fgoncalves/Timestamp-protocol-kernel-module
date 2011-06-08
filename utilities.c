#include <linux/kernel.h>
#include <linux/module.h>
#include "utilities.h"

#define  SECS_2_NSECS 1000000000L
#define  USECS_2_NSECS 1000L


/*
 * Obtain a 64 bit value representing the nanoseconds since the Epoch.
 */
s64 get_us_kernel_current_time_in_ns(void){
  struct timeval t;
  memset(&t, 0, sizeof(struct timeval));
  do_gettimeofday(&t);
  return ((int64_t) t.tv_sec) * SECS_2_NSECS + ((int64_t) t.tv_usec) * USECS_2_NSECS;
}

/*
 * This function is used to convert network byte order to host byte order and vice versa,
 * of a 64 bit variable.
 */
s64 swap_time_byte_order(s64 time){
  unsigned char* bytes = (unsigned char*) &time;
  uint32_t word;

  memcpy(&word, bytes, sizeof(uint32_t));
  word = htonl(word);
  memcpy(bytes, &word, sizeof(uint32_t));
  
  memcpy(&word, bytes + sizeof(uint32_t), sizeof(uint32_t));
  word = htonl(word);
  memcpy(bytes + sizeof(uint32_t), &word, sizeof(uint32_t));
  
  return * ((s64 *) bytes);
}
