#ifndef __BLOCKING_QUEUE__
#define __BLOCKING_QUEUE__

#include <semaphore.h>
#include <pthread.h>
#include <unistd.h>

typedef struct {
  /*s64*/ unsigned long long accumulated_time;
  unsigned int id;
  unsigned char samples[3];
} packet_t;

typedef struct{
  char source[22];
  packet_t packet;
} item;

struct queue_t{
  pthread_mutex_t mutex;
  sem_t empty;
  sem_t full;
  int producer_pointer;
  int consumer_pointer;
  int queue_size;
  item* buffer;
};

typedef struct queue_t queue;

extern struct queue_t* new_blocking_queue(int capcity);
extern void produce(struct queue_t* q, item i);
extern item consume(struct queue_t* q);
extern void destroy_queue(struct queue_t* q);
#endif
