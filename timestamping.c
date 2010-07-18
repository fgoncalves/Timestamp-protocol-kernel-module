#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef __MODULE__
#define __MODULE__
#endif

#ifndef CONFIG_NETFILTER
#define CONFIG_NETFILTER
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/vmalloc.h>
#include <linux/ip.h> 
#include <linux/netdevice.h> 
#include <linux/netfilter.h> 
#include <linux/netfilter_ipv4.h> 
#include <linux/skbuff.h> 
#include <linux/udp.h>
#include <linux/time.h>
#include <net/checksum.h>

#define alloc(TSIZE,TYPE)\
  (TYPE*) kmalloc(TSIZE * sizeof(TYPE), GFP_KERNEL);

#define dealloc(PTR)\
  kfree(PTR)

#define print printk

#define null NULL

#define hash_size 311

#define __udp_proto_id__ 17

#ifdef NF_IP_PRE_ROUTING
#define ip_pre_routing NF_IP_PRE_ROUTING
#else
#define ip_pre_routing NF_INET_PRE_ROUTING
#endif

#ifdef NF_IP_POST_ROUTING
#define ip_post_routing NF_IP_POST_ROUTING
#else
#define ip_post_routing NF_INET_POST_ROUTING
#endif

typedef struct t_node{
  __be32 ip; //ip in network byte order
  __be16 port;
  unsigned int packet_id;
  s64 in_time; //kernel time at which packet entered this node
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

hash_node* new_node(__be32 ip, __be16 port, unsigned int packID, s64 inTime){
  hash_node* new = alloc(1,hash_node);
  if(new == null){
    print("[new_node] Fatal error in vmalloc.\n");
    return null;
  }
  memcpy(& (new->ip), &ip, sizeof(__be32));
  memcpy(& (new->port), &port, sizeof(__be16));
  memcpy(& (new->packet_id), & packID, sizeof(unsigned int));
  memcpy(& (new->in_time), & inTime, sizeof(s64));
  new->before = null;
  new->next = null;
  return new;
  return null;
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

unsigned long hash_index(__be32 ip, __be16 port, unsigned int packID){
  unsigned long h = 0, g;
  
  unsigned char name[sizeof(__be32) + sizeof(__be16) + sizeof(unsigned int)];

  memcpy(name, &ip, sizeof(__be32));
  memcpy(name + 4, &port, sizeof(__be16));
  memcpy(name + 6, &packID, sizeof(unsigned int));

  while (*name){
    h = ( h << 4 ) + (*name)++;    
    if ((g = h & 0xF0000000L)) {h ^= g >> 17;}    
    h &= ~g;
  }
  
  return h % hash_size;
}

void put(table* t, __be32 ip, __be16 port, unsigned int packID, s64 inTime){
  hash_node* node = new_node(ip, port, packID, inTime);
  unsigned long index;

  if(node == null){
    return;
  }

  index = hash_index(node->ip, node->port, node->packet_id);

  if((*t)[index] == null){
    (*t)[index] = new_sentinel();
    //Error in kmalloc
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

hash_node* hash_lookup(table t, __be32 ip, __be16 port, unsigned int packID){
  hash_node* iterator;
  unsigned long index = hash_index(ip, port,packID);

  if(t[index] == null)
    return null;

  for(iterator = t[index]->first; iterator != null; iterator = iterator->next)
    if(memcmp(& (iterator->ip), &ip, sizeof(__be32)) == 0 
       && memcmp(& (iterator->port), &port, sizeof(__be16)) == 0 
       && memcmp(& (iterator->packet_id), & packID, sizeof(unsigned int)) == 0)
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
	print("[%d] ip: %u port: %hu packetID: %u inTime: %lld\n", i, iterator->ip, iterator->port, iterator->packet_id, iterator->in_time);
      }
      print("[%d] ********************** Hash row end **************************\n", i);
    }
  }
}

table __table;

static struct nf_hook_ops nf_ip_pre_routing;
static struct nf_hook_ops nf_ip_post_routing;

unsigned short service_port = 57843;

__be16 udp_checksum(struct iphdr* iphdr, struct udphdr* udphdr, unsigned char* data){
  __be32 sum = 0;
  __be16 proto = 0x1100; //17 udp
  __be16 data_length = (__be16) ntohs(udphdr->len) - sizeof(struct udphdr);
  __be16 src[2];
  __be16 dest[2];
  __be16 *padded_data;
  int padded_data_length, i;
  
  if(data_length % 2 != 0)
    padded_data_length = (int) data_length / 2 + 1;
  else
    padded_data_length = (int) data_length / 2;

  padded_data = alloc(padded_data_length, __be16);
  padded_data[padded_data_length - 1] = 0;
  memcpy(padded_data,data, data_length);

  src[0] = (__be16) (iphdr->saddr >> 16);
  src[1] = (__be16) (iphdr->saddr);
  dest[0] = (__be16) (iphdr->daddr >> 16);
  dest[1] = (__be16) (iphdr->daddr);


  data_length = (__be16) htons(data_length);


  sum = src[0] + src[1] + dest[0] + dest[1] + proto + udphdr->len + udphdr->source + udphdr->dest + udphdr->len;
  
  for(i = 0; i < padded_data_length; i++)
    sum += padded_data[i];
 
  while(sum >> 16)
    sum = (__be16) (sum & 0xFFFF) + (__be16) (sum >> 16);
  return (__be16) ~sum;
}

s64 get_kernel_current_time(void){
  struct timespec t = CURRENT_TIME;
  return timespec_to_ns(&t);
}

s64 swap_time_byte_order(s64 time){
  unsigned char* bytes = (unsigned char*) &time;
  uint32_t word;

  memcpy(&word, bytes, sizeof(uint32_t));
  word = htonl(word);
  memcpy(bytes, &word, sizeof(uint32_t));
  
  memcpy(&word, bytes + sizeof(uint32_t), sizeof(uint32_t));
  word = htonl(word);
  memcpy(bytes + sizeof(uint32_t), &word, sizeof(uint32_t));
  
  return * ((s64 *) bytes);
}

unsigned int nf_ip_pre_routing_hook(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff*)){
  struct iphdr* ip_header;
  struct udphdr* udp_header;
  unsigned char* transport_data;
  s64 time = 0;
  unsigned int id = 1;

  if(!skb){ 
    return NF_ACCEPT; 
  } 
  if(!(skb->network_header) || !(skb->transport_header)){ 
    return NF_ACCEPT; 
  }
  
  ip_header = ip_hdr(skb);
  if(ip_header->protocol != __udp_proto_id__){ 
    return NF_ACCEPT;
  }

  //  because there is a bug in the current kernel we can't simple do "udp_header = udp_hdr(skb);". Instead we have to do:
  udp_header = (struct udphdr*)(skb->data+(ip_header->ihl << 2));

  if((udp_header->source) == (unsigned short) htons(service_port)){ 
    time = get_kernel_current_time();
    transport_data = skb->data + sizeof(struct iphdr) + sizeof(struct udphdr);

    memcpy(&time, transport_data, sizeof(s64));
    memcpy(&id, transport_data + sizeof(s64), sizeof(unsigned int));

    //Because time is already in host byte order, conversion is not needed
    put(& __table, ip_header->saddr, udp_header->source, ntohl(id), time);
    return NF_ACCEPT;
  }
  
  return NF_ACCEPT;
}

unsigned int nf_ip_post_routing_hook(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff*)){
  struct iphdr* ip_header;
  struct udphdr* udp_header;
  unsigned char* transport_data;
  unsigned int id;
  hash_node* n;
  s64 total_accumulated_time, acc_time;
  
  if(!skb){ 
    return NF_ACCEPT; 
  } 
  if(!(skb->network_header) || !(skb->transport_header)){ 
    return NF_ACCEPT; 
  }
  
  ip_header = ip_hdr(skb);
  if(ip_header->protocol != __udp_proto_id__){ 
    return NF_ACCEPT;
  }

  //  because there is a bug in the current kernel we can't simple do "udp_header = udp_hdr(skb);". Instead we have to do:
  udp_header = (struct udphdr*)(skb->data+(ip_header->ihl << 2));

  if((udp_header->source) == (unsigned short) htons(service_port)){
    transport_data = skb->data + sizeof(struct iphdr) + sizeof(struct udphdr);
    memcpy(&acc_time, transport_data, 8);
    acc_time = swap_time_byte_order(acc_time);
    memcpy(&id, transport_data + 8, 4);

    n = hash_lookup(__table, ip_header->saddr, udp_header->source, ntohl(id));
    if(n != null){
      total_accumulated_time = acc_time + (get_kernel_current_time() - n->in_time);
      delete(& __table, n);
    }else //packet was created in node
      total_accumulated_time = get_kernel_current_time() - acc_time;

    total_accumulated_time = swap_time_byte_order(total_accumulated_time);

    memcpy(skb->data + sizeof(struct iphdr) + sizeof(struct udphdr), &total_accumulated_time, sizeof(s64));
    udp_header->check = udp_checksum(ip_header, udp_header, transport_data);

    if(!udp_header->check)
      udp_header->check = 0xFFFF;

    return NF_ACCEPT;
  }
  
  return NF_ACCEPT;
}

int init_module(){
  print("Packets are now being timestamped.\n");
  __table = new_table();
  //Error in kmalloc
  if(__table == null)
    return -1;

  nf_ip_pre_routing.hook = nf_ip_pre_routing_hook;
  nf_ip_pre_routing.pf = PF_INET;                              
  nf_ip_pre_routing.hooknum = ip_pre_routing;
  nf_ip_pre_routing.priority = NF_IP_PRI_FIRST;
  nf_register_hook(& nf_ip_pre_routing);

  nf_ip_post_routing.hook = nf_ip_post_routing_hook;
  nf_ip_post_routing.pf = PF_INET;
  nf_ip_post_routing.hooknum = ip_post_routing;
  nf_ip_post_routing.priority = NF_IP_PRI_FIRST;
  nf_register_hook(& nf_ip_post_routing);

  return 0;
}

void cleanup_module(){
  if(__table != null)
    explode_table(&__table);
  nf_unregister_hook(&nf_ip_pre_routing);
  nf_unregister_hook(&nf_ip_post_routing);
  print("Packets are no longer being timestamped.\n");
}
