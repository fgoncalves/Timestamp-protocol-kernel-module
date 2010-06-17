#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/vmalloc.h>

#define alloc(TSIZE,TYPE)\
  (TYPE*) vmalloc(TSIZE * sizeof(TYPE))

#define dealloc(PTR)\
  vfree(PTR)

#define print printk

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

table new_table(void){
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

static sentinel* new_sentinel(void){
  sentinel* s = alloc(1,sentinel);
  s->first = null;
  s->last = null;
  return s;
}

static unsigned long hash_index(unsigned char* ip, unsigned char* port, unsigned int packID){
  unsigned long h = 0, g;
  
  unsigned char name[6 + sizeof(unsigned int)];

  memcpy(name, ip, 4);
  memcpy(name + 4, port, 2);
  memcpy(name + 6, & packID, sizeof(unsigned int));

  while (*name){
    h = ( h << 4 ) + (*name)++;    
    if ((g = h & 0xF0000000L)) {h ^= g >> 17;}    
    h &= ~g;
  }
  
  return h % hash_size;
}

void put(table* t, unsigned char* ip, unsigned char* port, unsigned int packID, long inTime){
  hash_node* node = new_node(ip, port, packID, inTime);

  unsigned long index = hash_index(node->ip, node->port, node->packet_id);

  if((*t)[index] == null)
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

hash_node* hash_lookup(table t, unsigned char* ip, unsigned char* port, unsigned int packID){
  hash_node* iterator;
  unsigned long index = hash_index(ip, port,packID);

  if(t[index] == null)
    return null;

  for(iterator = t[index]->first; iterator != null; iterator = iterator->next)
    if(memcmp(iterator->ip, ip, 4) == 0 && memcmp(iterator->port, port, 2) == 0 && memcmp(& (iterator->packet_id), & packID, sizeof(unsigned int)) == 0)
      return iterator;

  return null;
}

void delete(table* t, hash_node* node){
  unsigned long index = hash_index(node->ip, node->port, node->packet_id);
  
  //Only one element in hash row
  if(node->next == null && node->before == null){
    dealloc(node);
    dealloc((*t)[index]);
    return;
  }

  //first element in hash row
  if(node->before == null){
    (*t)[index]->first = node->next;
    node->next->before = null;
    dealloc(node);
    return;
  }

  //last element in hash row
  if(node->next == null){
    (*t)[index]->last = node->before;
    node->before->next = null;
    dealloc(node);
    return;
  }

  //element in the middle of the row
  node->before->next = node->next;
  node->next->before = node->before;
  dealloc(node);
  return;
}

void explode_table(table* t){
  int i;
  hash_node* iterator, *aux;
  for(i = 0; i < hash_size; i++)
    if((*t)[i] != null)
      for(iterator = (*t)[i]->first; iterator != null;){
	aux = iterator;
	iterator = iterator->next;
	delete(t,aux);
      }
  dealloc(*t);
}

void dump_table(const table t){
  int i;
  hash_node* iterator;
  for(i = 0; i < hash_size; i++){
    if(t[i] != null){
      print("[%d] ********************** Hash row ******************************\n", i);
      for(iterator = t[i]->first; iterator != null; iterator = iterator->next){
	print("[%d] ip: %s port: %s packetID: %u inTime: %ld\n", i, iterator->ip, iterator->port, iterator->packet_id, iterator->in_time);
      }
      print("[%d] ********************** Hash row end **************************\n", i);
    }
  }
}

table t;

int init_module(){
  print("starting module timestamping");
  t = new_table();
  put(&t,"\x55\x55\x55\x55","\x22\x22",1,1234);
  put(&t,"\x55\x55\x55\x55","\x22\x22",1,1234);
  put(&t,"\x55\x55\x55\x55","\x22\x22",1,1234);
  put(&t,"\x55\x55\x55\x55","\x22\x22",1,1234);
  put(&t,"\x55\x55\x55\x55","\x22\x22",1,1234);
  put(&t,"\x55\x55\x55\x55","\x22\x22",1,1234);
  
  dump_table(t);
  return 0;
}

void cleanup_module(){
  explode_table(&t);
  print("Hash table freed. Shuting down timestamp module.");
}
