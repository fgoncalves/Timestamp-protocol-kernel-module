#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "macros.h"
#include "blocking_queue.h"

void swap_packet_byte_order(packet_t* packet){
  unsigned char* bytes = (unsigned char*) packet;

  uint32_t word;

  memcpy(&word, bytes, 4);
  word = htonl(word);
  memcpy(bytes, &word, 4);

  memcpy(&word, bytes + 4, 4);
  word = htonl(word);
  memcpy(bytes + 4, &word, 4);

  memcpy(&word, bytes + 8, 4);
  word = htonl(word);
  memcpy(bytes + 8, &word, 4);

  packet = (packet_t*) bytes;
  //TODO: samples need conversion?
}

static void semaphore_initialization_error_reason(){
  switch(errno){
  case EINVAL:
    print_error("\tThe value argument exceeds SEM_VALUE_MAX\n");
    break;
  case ENOSPC:
    print_error("\tA resource required to initialize the semaphore has been exhausted\n");
    break;
  case ENOSYS:
    print_error("\tThis system does not support sem_init\n");
    break;
  case EPERM:
    print_error("\tThe process does not have the privileges to run the semaphore\n");
    break;
  default:
    print_error("\tUnkown reason\n");
  }
}

static void mutex_initialization_error_reason(){
  switch(errno){
  case EAGAIN:
    print_error("\tLack of resources\n");
    break;
  case ENOMEM:
    print_error("\tInsufficient memory\n");
    break;
  case EPERM:
    print_error("\tThe process does not have the privileges to perform pthread_mutex_init\n");
    break;
  case EBUSY:
    print_error("\tRe-initialization of a mutex that has not been destroyed\n");
    break;
  case EINVAL:
    print_error("The value specified by attr is invalid\n");
    break;
  }
}

queue* new_blocking_queue(int capacity){
  queue* nova;
  int status;
  alloc(nova,queue,1);

  status = pthread_mutex_init(& (nova->mutex), null);
  if(status != 0){
    print_error("##Failed to initialize mutex of blocking queue.\n");
    mutex_initialization_error_reason();
    exit(EXIT_ERROR);
  }
  
  status = sem_init(& (nova->empty), 0, capacity);
  if(status == -1){
    print_error("##Failed to initialize empty semaphore of blocking queue.\n");
    semaphore_initialization_error_reason();
    exit(EXIT_ERROR);
  }

  status = sem_init(& (nova->full), 0, 0);
  if(status == -1){
    print_error("##Failed to initialize full semaphore of blocking queue.\n");
    semaphore_initialization_error_reason();    
    exit(EXIT_ERROR);
  }

  nova->producer_pointer = 0;
  nova->queue_size = capacity;
  nova->consumer_pointer = 0;
  alloc((nova->buffer),item,capacity);
  return nova;
}

void produce(queue* q, item i){
  sem_wait(& (q->empty));
  pthread_mutex_lock(& (q->mutex));
  q->buffer[q->producer_pointer++] = i;
  q->producer_pointer %= q->queue_size;
  pthread_mutex_unlock(& (q->mutex));
  sem_post(& (q->full));
}

item consume(queue* q){
  sem_wait(& (q->full));
  pthread_mutex_lock(& (q->mutex));
  item i = q->buffer[q->consumer_pointer++];
  q->consumer_pointer %= q->queue_size;
  pthread_mutex_unlock(& (q->mutex));
  sem_post(& (q->empty));
  return i;
}

void destroy_queue(queue* q){
  free(q->buffer);
  pthread_mutex_destroy(& (q->mutex));
  sem_destroy(& (q->full));
  sem_destroy(& (q->empty));
  free(q);
}
