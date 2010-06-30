#ifndef __SOCKET_LIST_H__
#define __SOCKET_LIST_H__

struct socket_node_t{
  struct sockaddr_in* sock;
  struct socket_node_t* next;
};

struct socket_list_t{
  int server_socket_fd;
  struct socket_node_t* first;
  struct socket_node_t* last;
  int nelems;
};

struct socket_list_iterator_t{
  struct socket_node_t* current;
};

typedef struct socket_node_t node;
typedef struct socket_list_t socket_list;
typedef struct socket_list_iterator_t iterator;

extern socket_list* new_socket_list();
extern void add_element(socket_list* list, unsigned short destination_port, char* destination_ip);
extern iterator* new_iterator(socket_list* list);
extern node* next(iterator *it);
extern int  has_next(iterator* it);
extern void destroy_iterator(iterator* t);
extern void destroy_list(socket_list *list);
#endif
