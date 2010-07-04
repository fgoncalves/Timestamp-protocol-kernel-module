#include <stdio.h>
#include <stdlib.h>
#include <string.h> /*memcpy*/
#include <errno.h>
#include <unistd.h>
#include <pthread.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "blocking_queue.h"
#include "thread_functions.h"
#include "macros.h"
#include "statistics.h"
#include "timestamp.h"

unsigned int npackets = 0;
queue *sending_queue, *receiving_queue;
pthread_mutex_t lock;
unsigned int samples_per_second = 25;
unsigned int usleep_time;
char is_sink = 0;

static void determine_send_error_reason(){
  switch(errno){
  case EAFNOSUPPORT:
    print_error("\tThe socket does not support the given address family\n");
    break;
  case EAGAIN:
    print_error("\tSocket's fd is marked has non blocking but the operation requires blocking on it\n");
    break;
  case EBADF:
    print_error("\tsocket argument is not a valid fd\n");
    break;
  case ECONNRESET:
    print_error("\tThe connection was forcibly closed by a peer\n");
    break;
  case EINTR:
    print_error("\tA signal interrupted sendto before any data was transmitted\n");
    break;
  case EMSGSIZE:
    print_error("\tThe message is to large to send all at once\n");
    break;
  case ENOTCONN:
    print_error("\tThe socket is connection-mode but is not connected\n");
    break;
  case ENOTSOCK:
    print_error("\tThe socket argument does not refer to a socket\n");
    break;
  case EOPNOTSUPP:
    print_error("\tThe socket argument is associated with a socket that does not support one or more of the values set in flags\n");
    break;
  case EPIPE:
    print_error("\tBroken pipe: socket sutdown for writing or is not connected\n");
    break;
  default:
    print_error("\tUnknown reason\n");
  }
}

static void send_bytes(int socketFD, const char* buff, int length,struct sockaddr* sock, int sockLength){
  if(sendto(socketFD, buff, length, 0, sock, sockLength) == -1){
    print_error("##Error sending packet\n");
    determine_send_error_reason();
  }
}

static void send_to_all(packet_t Item, socket_list *destination_sockets){
  iterator *iter;
  node* aux;
  byte buffer[sizeof(packet_t)];
  
  iter = new_iterator(destination_sockets);
  while(has_next(iter)){
    aux = next(iter);
    memcpy(buffer, & Item, sizeof(packet_t));
    send_bytes(destination_sockets->server_socket_fd, buffer, sizeof(packet_t), (struct sockaddr *) aux->sock, sizeof(struct sockaddr_in));
  }
  destroy_iterator(iter);
}

static void init_packet(packet_t* packet){
  static unsigned int id = 0;
  struct timespec ts;
  unsigned long long creation_timestamp;

  packet->id = id;
  id++;
  clock_gettime(CLOCK_REALTIME, &ts);
  creation_timestamp = timespec_to_ns(ts);
  memcpy(& (packet->accumulated_time), & creation_timestamp, sizeof(unsigned long long));
}

void send_packet(socket_list *destination_sockets){
  packet_t anything;

  printf("Sampling at %d samples per second.\n", samples_per_second);

  usleep_time = (unsigned int) 1000000 / samples_per_second;

  while(npackets > 0){
    memset(&anything, 0, sizeof(anything));
    init_packet(&anything);

    send_to_all(anything, destination_sockets);
    npackets--;
    usleep(usleep_time);
  }

  printf("All generated packets were sent.\n");
  pthread_exit(0);
}

void treat_packet(socket_list * destination_sockets){
  item i;

  while(true){
    i = consume(receiving_queue);
    pthread_mutex_lock(& lock);
    
    if(is_sink){;
      write_data(i.in_time, i.packet.id, i.source);
    }

    pthread_mutex_unlock(& lock);
    if(destination_sockets->nelems > 0)
      send_to_all(i.packet, destination_sockets);
  }
}
