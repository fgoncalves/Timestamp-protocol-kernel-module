#ifndef __UTILITIES_H__
#define __UTILITIES_H__

#include <linux/vmalloc.h>

#define alloc(TSIZE,TYPE)\
  (TYPE*) kmalloc(TSIZE * sizeof(TYPE), GFP_KERNEL);

#define dealloc(PTR)\
  kfree(PTR)

#define print printk

#define null NULL

extern s64 get_us_kernel_current_time_in_ns(void);
extern s64 swap_time_byte_order(s64 time);

#ifdef DEBUG
#define debug(format, ...)					\
  do{								\
    printk("%s in %s:%u: ", __FUNCTION__, __FILE__, __LINE__);	\
    printk(format, ## __VA_ARGS__);				\
  }while(0);
#else							
#define debug(format, ...)					
#endif

#endif
