#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define alloc(TSIZE,TYPE)\
  (TYPE*) malloc(TSIZE * sizeof(TYPE))

#define dealloc(PTR)\
  free(PTR)

#define print printf

#define null NULL

#define hash_size 311

typedef struct t_node{
  unsigned char ip[4]; //ip in network byte order
  unsigned char port[2];
  unsigned int packet_id;
  long in_time; //time since epoch when packet entered this node
  struct t_node* before;
  struct t_node* next;
} hash_node;

typedef struct t_sentinel{
  struct t_node* first;
  struct t_node* last;
} sentinel;

typedef sentinel** table;

table new_table(){
  int i;
  table t = alloc(hash_size,sentinel*);
  for(i = 0; i < hash_size; i++)
    t[i] = null;
  return t;
}

static hash_node* new_node(unsigned char *ip, unsigned char* port, unsigned int packID, long inTime){
  hash_node* new = alloc(1,hash_node);
  memcpy(new->ip, ip, 4);
  memcpy(new->port, port, 2);
  memcpy(& (new->packet_id), & packID, sizeof(unsigned int));
  memcpy(& (new->in_time), & inTime, sizeof(long));
  new->before = null;
  new->next = null;
  return new;
}

static sentinel* new_sentinel(){
  sentinel* s = alloc(1,sentinel);
  return s;
}

static unsigned long hash_index(const hash_node node){
  unsigned long h = 0, g;
  
  unsigned char name[6 + sizeof(unsigned int)];

  memcpy(name, node.ip, 4);
  memcpy(name + 4, node.port, 2);
  memcpu(name + 6, node.packet_id, sizeof(unsigned int));

  while (*name){
    h = ( h << 4 ) + (*name)++;    
    if (g = h & 0xF0000000L) {h ^= g >> 24;}    
    h &= ~g;
  }
  
  return h % hash_size;
}

void put(table* t, unsigned char* ip, unsigned char* port, unsigned int packID, long inTime){
  hash_node* node = new_node(ip, port, packID, inTime);

  unsigned long index = hash_index(*node);

  if((*t)[index])
    (*t)[index] = new_sentinel();

  if((*t)[index]->first == null){
    (*t)[index]->first = node;
    (*t)[index]->last = node;
    return;
  }
  
  node->before = (*t)[index]->last;
  (*t)[index]->last->next = node;
  (*t)[index]->last = node;
  return;
}



void dump_table(table t){
  int i;
  hash_node* iterator;
  for(i = 0; i < hash_size; i++)
    if(t[i] != null){
      print("list in normal order\n");
      for(iterator = t[i]->first; iterator != null; iterator = iterator->next){
	print("ip: %s port: %s packetID: %ud inTime: %ld\n", iterator->ip, iterator->port, iterator->packet_id, iterator->in_time);
      }
      print("list in reverse\n");
      for(iterator = t[i]->last; iterator != null; iterator = iterator->before){
	print("ip: %s port: %s packetID: %ud inTime: %ld\n", iterator->ip, iterator->port, iterator->packet_id, iterator->in_time);
      }
    }
}
