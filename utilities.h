#ifndef __UTILITIES_H__
#define __UTILITIES_H__

#include <linux/vmalloc.h>

#define alloc(TSIZE,TYPE)\
  (TYPE*) kmalloc(TSIZE * sizeof(TYPE), GFP_KERNEL);

#define dealloc(PTR)\
  kfree(PTR)

#define print printk

#define null NULL

extern s64 get_kernel_current_time(void);
extern s64 swap_time_byte_order(s64 time);

#endif
