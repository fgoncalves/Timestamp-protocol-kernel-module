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
#include "packet.h"
#include "statistics.h"
#include "macros.h"

/*global*/ unsigned int npackets = 0;
/*global*/ unsigned short sink_dport = 0;
/*global*/ char sink_dip[16] = {0};

typedef int socket_fd;

void parse_input(char* input, char* ip /*out*/, unsigned short* port /*out*/){
  char *token;
  token = strtok(input,": \n");
  strncpy(ip, token, 16);
  token = strtok(null,": \n");
  *port = (unsigned short) atoi(token);
}

void init_destination(){
  char ip[22];

  scanf("%u", &npackets);

  memset(ip, 0, 22);
  scanf("%s",ip);
  parse_input(ip, sink_dip, &sink_dport);
}

void serve(int server_fd){
  char buff[sizeof(packet_t)];
  socklen_t client_length;  
  char source_ip_port[22] = {0};
  packet_t pk;
  struct sockaddr_in client;

  while(true){
    memset(buff, 0, sizeof(packet_t));
    memset(&client, 0, sizeof(client));

    client_length = sizeof(client);

    if(recvfrom(server_fd, buff, sizeof(packet_t), 0,(struct sockaddr *) & client, &client_length) != -1){
      memcpy(& pk, buff, sizeof(packet_t));
      
      swap_packet_byte_order(& pk);

      memset(source_ip_port,0, 22);
      sprintf(source_ip_port, "%s:%hu", inet_ntoa(client.sin_addr), ntohs(client.sin_port));
      write_data(pk.accumulated_time, pk.in_time, pk.id, source_ip_port, pk.samples);
    }
    else{
      log(E, "##Fatal error: recvfrom returned without any packet arrival\n");
      exit(-1);
    }
  }
}

void collect(socket_fd client_socket){
  packet_t anything;
  unsigned int usleep_time = (unsigned int) 1000000 / samples_per_second;
  struct sockaddr_in* sock = alloc(struct sockaddr_in, 1);
  char buffer[sizeof(packet_t)];
 
  sock->sin_family = AF_INET;
  sock->sin_port = htons(sink_dport);
  sock->sin_addr.s_addr = inet_addr(sink_dip);

  while(npackets > 0){
    memset(&anything, 0, sizeof(anything));
    init_packet(&anything);

    write_data(anything.accumulated_time, anything.in_time, anything.id, "me", anything.samples);
    
    swap_packet_byte_order(&anything);

    memcpy(buffer, &anything, sizeof(packet_t)); 

    if(sendto(client_socket, buffer, sizeof(packet_t), 0,(const struct sockaddr *) sock, sizeof(struct sockaddr_in)) == -1){
      log(E, "Could not send packet\n");
    }

    npackets--;

    usleep(usleep_time);
  }

  printf("Simulation has ended.\n");
}

void cleanup(){
  printf("\n\nSimulation has ended.\nStatistics file is %s\n", get_statistics_file());
  close_statistics();
  exit(0);
}

unsigned int parse_samples_per_second(char* samples){
  int i;
  /*sanity check*/
  for(i = 0; samples[i] != '\0'; i++)
    if(!isdigit(samples[i])){
      log(E,"Invalid samples per second %s\n", samples);
      exit(-1);
    }
  return (unsigned int) atoi(samples);
}

int main(int argc, char** argv){
  struct sockaddr_in sock;
  int my_sock_fd, i;
  unsigned short port = 57843;
  char is_sink = false;

  if(argc < 2){
    printf("Usage: %s <ip:port to bind> [samples_per_second | -e] [-s]\n",argv[0]);
    exit(-1);
  }

  if(argc > 2){
    for(i = 2; i < argc; i++){
      if(strcmp(argv[i], "-e") == 0){
	emulate_ts = true;
	continue;
      }
      if(strcmp(argv[i], "-s") == 0){
	is_sink = true;
	continue;
      }
      samples_per_second = parse_samples_per_second(argv[i]);
    }
  }

  if(is_sink)
    printf("Acting as sink.\n");

  my_sock_fd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
  if(my_sock_fd == -1){
    log(E, "Cannot open my socket\n");
    exit(-1);
  }

  memset(& sock, 0, sizeof(sock));
  sock.sin_family = AF_INET;
  sock.sin_port = htons(port);
  sock.sin_addr.s_addr = inet_addr(argv[1]);
  if(sock.sin_addr.s_addr == (in_addr_t) (-1)){
    log(E, "Unable to parse ip %s.\n", argv[1]);
    exit(-1);
  }

  if(bind(my_sock_fd, (struct sockaddr *) &sock, sizeof(sock)) == -1){
    log(E, "Unnable to bind socket\n");
    exit(-1);
  }

  init_statistics();
  signal(SIGINT,(void*) cleanup);
  if(is_sink){
    serve(my_sock_fd);
  }
  else{
    init_destination();
    if(emulate_ts)
      printf("Emulating timestamp.\n");
    printf("Sampling at %u\n", samples_per_second);
    collect(my_sock_fd);
  }
  return 0;
}
