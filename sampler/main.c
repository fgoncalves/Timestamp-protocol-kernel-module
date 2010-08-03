#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <ctype.h>
#include "blocking_queue.h"
#include "macros.h"
#include "socket_list.h"
#include "thread_functions.h"
#include "statistics.h"

typedef int socket_fd;

#define MAX_RECEIVING_THREADS 5

socket_list *destination_sockets;
pthread_t receiving_threads[MAX_RECEIVING_THREADS], sending_thread;

void parse_input(char* input, char* ip /*out*/, unsigned short* port /*out*/){
  char *token;
  token = strtok(input,": \n");
  strncpy(ip, token, 16);
  token = strtok(null,": \n");
  *port = (unsigned short) atoi(token);
}

unsigned int parse_samples_per_second(char* samples){
  int i;
  /*sanity check*/
  for(i = 0; samples[i] != '\0'; i++)
    if(!isdigit(samples[i])){
      print_error("Invalid 'samples_per_second' parameter.\n");
      exit(EXIT_ERROR);
    }
  return (unsigned int) atoi(samples);
}

void init_receiving_threads(){
  int i;
  for(i = 0; i < MAX_RECEIVING_THREADS; i++)
    pthread_create(& receiving_threads[i], null, (void*) treat_packet,(void *) destination_sockets);
}

void init_sending_thread(){
  pthread_create(& sending_thread, null, (void *) send_packet, (void *) destination_sockets);
}

void init_destinations(int server_fd){
  unsigned short dport;
  char ips[22], dip[16];

  destination_sockets = new_socket_list(server_fd);

  while(true){
    memset(ips, 0, 22);
    scanf("%s",ips);

    if(strcmp(ips,"END") == 0)
      break;

    parse_input(ips, dip, &dport);
    
    add_element(destination_sockets, dport, dip);
    memset(dip, 0, 16);
  }

  if(destination_sockets->nelems > 0)
    init_sending_thread();
  
  init_receiving_threads();
}

void cleanup(){
  printf("\n\nSimulation has ended.\nStatistics file is %s\n", get_statistics_file());
  close_statistics();
  exit(0);
}

void init_all(int server_fd){
  init_statistics();
  signal(SIGINT,(void*) cleanup);

  scanf("%u",&npackets);
  
  pthread_mutex_init(&lock, null);
  receiving_queue = new_blocking_queue(15);
  init_destinations(server_fd);
}

void determine_bind_error_reason(){
  switch(errno){
  case EACCES:
    printf("The requested address is protected, and the current user has inadequate permission to access it.\n");
    printf("On an Unix system it may be that a component of the path prefix does not allow searching or the node's parent directory denies write permission.\n");
    break;
  case EADDRINUSE:
    printf("The specified address is already in use.\n");
    break;
  case EADDRNOTAVAIL:
    printf("The specified address is not available from the local machine.\n");
    break;
  case EAFNOSUPPORT:
    printf("Address is not valid for the address family of socket.\n");
    break;
  case EBADF:
    printf("Socket is not a valid file descriptor.\n");
    break;
  case EDESTADDRREQ:
    printf("Socket is a null pointer.\n");
    break;
  case EFAULT:
    printf("The address parameter is not in a valid part of the user address space.\n");
    break;
  case EINVAL:
    printf("Socket is already bound to an address and the protocol does not support binding to a new address.  Alternatively, socket may have been shut down.\n");
    break;
  case ENOTSOCK:
    printf("Socket does not refer to a socket.\n");
    break;
  case EOPNOTSUPP:
    printf("Socket is not of a type that can be bound to an address.\n");
    break;
  case EIO:
    printf("An I/O error occurred while making the directory entry or allocating the inode.\n");
    break;
  case EISDIR:
    printf("An empty pathname was specified.\n");
    break;
  case ELOOP:
    printf("Too many symbolic links were encountered in translating the pathname.  This is taken to be indicative of a looping symbolic link.\n");
    break;
  case ENAMETOOLONG:
    printf("A component of a pathname exceeded {NAME_MAX} characters, or an entire path name exceeded {PATH_MAX} characters.\n");
    break;
  case ENOENT:
    printf("A component of the path name does not refer to an existing file.\n");
    break;
  case ENOTDIR:
    printf("A component of the path prefix is not a directory.\n");
    break;
  case EROFS:
    printf("The name would reside on a read-only file system.\n");
    break;
  default:
    printf("Unknown reason.\n");
  }
}

void serve(int server_fd){
  byte buff[sizeof(packet_t)];
  socklen_t client_length;  
  char source_ip_port[22] = {0};
  item it;
  struct sockaddr_in client;

  while(true){
    memset(buff, 0, sizeof(packet_t));
    memset(&client, 0, sizeof(client));

    client_length = sizeof(client);

    memset(&it, 0, sizeof(it));
    if(recvfrom(server_fd, buff, sizeof(packet_t), 0,(struct sockaddr *) & client, &client_length) != -1){
      memcpy(& (it.packet), buff, sizeof(packet_t));
      
      swap_packet_byte_order(& (it.packet));

      memset(source_ip_port,0, 22);
      sprintf(source_ip_port, "%s:%hu", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
      memcpy(it.source, source_ip_port, 22); 
      produce(receiving_queue, it);
    }
    else{
      print_error("##Fatal error: recvfrom returned without any packet arrival\n");
      exit(EXIT_ERROR);
    }
  }
}


int main(int argc, char** argv){
  struct sockaddr_in server;
  int server_fd;
  unsigned short port;
  char ip[16] = {0};

  switch (argc){
  case 2:
    break;
  case 3:
    if(strcmp(argv[2],"-sink") == 0)
      is_sink = true;
    else
      samples_per_second = parse_samples_per_second(argv[2]);
    break;
  case 4:
    if(strcmp(argv[2],"-sink") == 0){
      is_sink = true;
      samples_per_second = parse_samples_per_second(argv[3]);
    }
    else{
      samples_per_second = parse_samples_per_second(argv[2]);
      if(strcmp(argv[3], "-sink") == 0)
	is_sink = true;
      else{
	printf("Invalid option %s\n", argv[3]);
	exit(EXIT_ERROR);
      }	
    }
    break;
  default:
    printf("Usage: %s <ip:port to bind> [samples_per_second] [-sink]\n",argv[0]);
    exit(EXIT_ERROR);
  }

  parse_input(argv[1],ip,&port);

  server_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(server_fd == -1){
    print_error("##Cannot open server socket\n");
    exit(EXIT_ERROR);
  }

  memset(& server, 0, sizeof(server));
  server.sin_family = AF_INET;
  server.sin_port = htons(port);
  server.sin_addr.s_addr = inet_addr(ip);
  if(server.sin_addr.s_addr == (in_addr_t) (-1)){
    print_error("##Unable to parse server ip.\n");
    exit(EXIT_ERROR);
  }

  if(bind(server_fd, (struct sockaddr *) &server, sizeof(server)) == -1){
    print_error("##Unnable to bind server socket\n");
    determine_bind_error_reason();
    exit(EXIT_ERROR);
  }

  init_all(server_fd);
  serve(server_fd);
  return 0;
}
