#ifndef __MACROS_H__
#define __MACROS_H__

#define print_error(str) \
fprintf(stderr,str); \
fflush(stderr)

#define EXIT_ERROR -1

#define alloc(PTR,TYPE,QUANT)			\
  PTR = (TYPE*) malloc(sizeof(TYPE) * QUANT);	\
  if(PTR == NULL){				\
    fprintf(stderr,"##Fatal error in malloc");	\
    exit(-1);					\
  }

#define null NULL
#define true 1
#define false 0

#endif
