#include <stdlib.h>
#include <stdio.h>
#include <string.h> 
#include <sys/socket.h>
#include <arpa/inet.h>
#include "socket_list.h"
#include "macros.h"

socket_list* new_socket_list(int socket_fd){
  socket_list* nova;
  alloc(nova,socket_list,1);
  nova->server_socket_fd = socket_fd;
  nova->first = null;
  nova->last = null;
  nova->nelems = 0;
  return nova;
}

void add_element(socket_list *list, unsigned short destination_port, char* destination_ip){
  node* novo;
  alloc(novo,node,1);
  alloc(novo->sock,struct sockaddr_in,1);

  memset(novo->sock,0,sizeof(struct sockaddr_in));
  novo->sock->sin_family = AF_INET;
  novo->sock->sin_port = htons(destination_port);
  novo->sock->sin_addr.s_addr = inet_addr(destination_ip);
  novo->next = null;

  if(list->first == null){
    list->first = novo;
    list->last = novo;
    list->nelems++;
    return;
  }

  list->nelems++;
  list->last->next = novo;
  list->last = novo;
  return;
}

iterator* new_iterator(socket_list* list){
  iterator *it;
  alloc(it, iterator, 1);
  it->current = list->first;
  return it;
}

node* next(iterator *it){
  if(it == null){
    print_error("##Null iterator\n");
    exit(EXIT_ERROR);
  }
  
  if(it->current == null){
    print_error("##Iterator does have a next element\n");
    exit(EXIT_ERROR);
  }

  node* ret = it->current;
  it->current = it->current->next;
  return ret;
}

int  has_next(iterator* it){
 if(it == null){
    print_error("##Null iterator\n");
    exit(EXIT_ERROR);
  }  
 return it->current != null;
}

void destroy_iterator(iterator* it){
  free(it);
}

void destroy_list(socket_list* list){
  node* aux = list->first;
  while(aux != null){
    list->first = aux->next;
    free(aux->sock);
    free(aux);
    aux = list->first;
  }
  free(list);
}
