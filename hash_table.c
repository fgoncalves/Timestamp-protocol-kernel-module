#include <stdio.h>
#include <stdlib.h>

#define alloc(TSIZE,TYPE)\\
  (TYPE*) malloc(TSIZE * sizeof(TYPE))

#define dealloc(PTR)\\
  free(PTR)

#define null NULL

#define hash_size 311

typedef struct t_node{
  char ip[4]; //ip in network byte order
  unsigned int packetID; 
  long in_time; //time since epoch when packet entered this node
  struct t_node* before;
  struct t_node* after;
} hash_node;

typedef struct t_sentinel{
  struct t_node* first;
  struct t_node* last;
} sentinel;

typedef sentinel** table;

table new_table(){
  int i;
  table = alloc(hash_size,sentinel*);
  for(i = 0; i < hash_size; i++)
    table[i] = null;
}
