#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/ip.h> 
#include <linux/netdevice.h> 
#include <linux/netfilter.h> 
#include <linux/netfilter_ipv4.h> 
#include <linux/skbuff.h> 
#include <linux/udp.h>

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
  if(t == null){
    print("[new_table] Fatal error in vmalloc.\n");
    return null;
  }

  for(i = 0; i < hash_size; i++)
    t[i] = null;
  return t;
}

hash_node* new_node(unsigned char *ip, unsigned char* port, unsigned int packID, long inTime){
  hash_node* new = alloc(1,hash_node);
  if(new == null){
    print("[new_node] Fatal error in vmalloc.\n");
    return null;
  }
  memcpy(new->ip, ip, 4);
  memcpy(new->port, port, 2);
  memcpy(& (new->packet_id), & packID, sizeof(unsigned int));
  memcpy(& (new->in_time), & inTime, sizeof(long));
  new->before = null;
  new->next = null;
  return new;
}

sentinel* new_sentinel(void){
  sentinel* s = alloc(1,sentinel);
  if(s == null){
    print("[new_sentinel] Fatal error in vmalloc.\n");
    return null;
  }
  s->first = null;
  s->last = null;
  return s;
}

unsigned long hash_index(unsigned char* ip, unsigned char* port, unsigned int packID){
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
  unsigned long index;

  if(node == null){
    return;
  }

  index = hash_index(node->ip, node->port, node->packet_id);

  if((*t)[index] == null){
    (*t)[index] = new_sentinel();
    //Error in vmalloc
    if((*t)[index] == null)
      return;
  }

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

static struct nf_hook_ops nf_ip_pre_routing;
static struct nf_hook_ops nf_ip_post_routing;
static struct nf_hook_ops nf_ip_local_out;

unsigned char* service_port = "\xE1\xF3";

unsigned int nf_ip_pre_routing_hook(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff*)){
  print("nf_ip_pre_routing_hook\n");
  return NF_ACCEPT;
}

unsigned int nf_ip_post_routing_hook(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff*)){
  print("nf_ip_post_routing_hook\n");
  return NF_ACCEPT;
}

unsigned int nf_ip_local_out_hook(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff*)){
  print("nf_ip_local_out_hook\n");
  return NF_ACCEPT;
}

int init_module(){
  print("starting module timestamping");
  t = new_table();
  //Error in vmalloc
  if(t == null)
    return -1;

  nf_ip_pre_routing.hook = nf_ip_pre_routing_hook;
  nf_ip_pre_routing.pf = PF_INET;                              
  nf_ip_pre_routing.hooknum = NF_INET_PRE_ROUTING;
  nf_ip_pre_routing.priority = NF_IP_PRI_FIRST;
  nf_register_hook(& nf_ip_pre_routing);

  nf_ip_post_routing.hook = nf_ip_post_routing_hook;
  nf_ip_post_routing.pf = PF_INET;
  nf_ip_post_routing.hooknum = NF_INET_POST_ROUTING;
  nf_ip_post_routing.priority = NF_IP_PRI_FIRST;
  nf_register_hook(& nf_ip_post_routing);

  nf_ip_local_out.hook = nf_ip_local_out_hook;
  nf_ip_local_out.pf = PF_INET;
  nf_ip_local_out.hooknum = NF_INET_LOCAL_OUT;
  nf_ip_local_out.priority = NF_IP_PRI_FIRST;
  nf_register_hook(& nf_ip_local_out);

  return 0;
}

void cleanup_module(){
  if(t != null)
    explode_table(&t);
  nf_unregister_hook(&nf_ip_pre_routing);
  nf_unregister_hook(&nf_ip_post_routing);
  nf_unregister_hook(&nf_ip_local_out);
  print("Hash table freed. Shuting down timestamp module.");
}
