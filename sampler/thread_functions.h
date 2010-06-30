#ifndef __THREAD_FUNCTIONS__
#define __THREAD_FUNCTIONS__

#include "socket_list.h"

typedef char byte;

extern unsigned int npackets, samples_per_second;
extern queue *receiving_queue;
extern pthread_mutex_t lock;
extern char is_sink;

extern void send_packet(socket_list* destination_sockets);
extern void treat_packet(socket_list* destination_sockets);
#endif
