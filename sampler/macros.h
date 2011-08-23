#ifndef __MACROS_H__
#define __MACROS_H__

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#ifdef DEBUG
#define debug(level, format, ...)					\
  do{									\
    fprintf(stderr, "%s  %s:%d in %s: ", level, __FILE__, __LINE__, __PRETTY_FUNCTION__); \
    fprintf(stderr, format, ## __VA_ARGS__);				\
  }while(0)
#else
#define debug(level, str, ...)
#endif

#define log(level, format, ...)						\
  do{									\
    fprintf(stderr, "%s  %s:%d in %s: ", level, __FILE__, __LINE__, __PRETTY_FUNCTION__); \
    fprintf(stderr, format, ## __VA_ARGS__);				\
  }while(0)

#ifdef BASH
#define I "\x1B[1;34;34m[INFO]\x1B[0;0;0m"
#define W "\x1B[1;33;33m[WARNNING]\x1B[0;0;0m"
#define E "\x1B[1;31;31m[ERROR]\x1B[0;0;0m"
#define F "\x1B[1;31;31m[FATAL ERROR]\x1B[0;0;0m"
#else
#define I "[INFO]"
#define W "[WARNNING]"
#define E "[ERROR]"
#define F "[FATAL ERROR]"
#endif

#define alloc(type, how_many)						\
  (type *) __alloc(malloc((how_many)*sizeof(type)), (how_many)*sizeof(type));

#define alloc_bytes(type, how_many_bytes)			\
  (type *) __alloc(malloc((how_many_bytes)), (how_many_bytes));

static inline void* __alloc(void* x, uint32_t len){
  if(x){
    memset(x, 0, len);
    return x;
  }
  log(F,"calloc failed.");
  exit(-1);
  return 0;
}

#define true 1
#define false 0
#define null NULL

#endif
