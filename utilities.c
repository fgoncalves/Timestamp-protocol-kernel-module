#include <linux/kernel.h>
#include <linux/module.h>
#include "utilities.h"

/*
 * Obtain a 64 bit value representing the nanoseconds since the Epoch.
 */
s64 get_kernel_current_time(void){
  struct timespec t = CURRENT_TIME;
  return timespec_to_ns(&t);
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
