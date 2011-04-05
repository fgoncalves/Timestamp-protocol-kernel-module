#include <linux/udp.h>
#include <linux/in.h>
#include "packet_tree.h"
#include "kpacket.h"

static  void* packet_key(struct stree_node* node){
  struct iphdr* ip = (struct iphdr*) node->node;
  packet_t* p = (packet_t*) (((char*) ip) + (ip->ihl << 2) + sizeof(struct udphdr));
  return &(p->id);
}

static int64_t packet_compare(void* keyA, void* keyB){
  uint32_t a = *(uint32_t*) keyA, b = *(uint32_t*) keyB;
  a = ntohl(a);
  b = ntohl(b);
  return ((int64_t) a) - ((int64_t) b);
}

packet_tree* new_packet_tree(void){
  packet_tree* pkt_t = (packet_tree*) kmalloc(sizeof(packet_tree), GFP_ATOMIC);

  if(!pkt_t){
    printk("%s in %s:%d: kmalloc failed\n", __FUNCTION__, __FILE__, __LINE__);
    return NULL;
  }

  pkt_t->npackts = 0;
  pkt_t->rbt = new_rbtree(packet_key, packet_compare);
  return pkt_t;
}

void free_packet_tree(packet_tree* pkt_t){
  tree_iterator* it = new_tree_iterator(pkt_t->rbt);
  while(tree_iterator_has_next(it)){
    char* p = (char*) tree_iterator_next(it);
    kfree(p);
  }
  destroy_iterator(it);
  destroy_rbtree(pkt_t->rbt);
  kfree(pkt_t);
}

int8_t store_packet_in_tree(packet_tree* pkt_t, struct iphdr* pkt){
  char* p = (char*) kmalloc(ntohs(pkt->tot_len), GFP_ATOMIC);

  if(!p){
    printk("%s in %s:%d: kmalloc failed\n", __FUNCTION__, __FILE__, __LINE__);
    return 0;
  }

  memcpy(p, pkt, ntohs(pkt->tot_len));
  
  rb_tree_insert(pkt_t->rbt, p);
  pkt_t->npackts++;
  return 1;
}

struct iphdr* get_packet_from_tree(packet_tree* pkt_t, uint32_t id){
  return (struct iphdr*) search_rbtree(*(pkt_t->rbt), &id);
}

void remove_packet_from_tree(packet_tree* pkt_t, uint32_t id){
  struct iphdr* ip = (struct iphdr*) rb_tree_delete(pkt_t->rbt, &id);
  if(ip)
    kfree(ip);
}

struct iphdr* discard_oldest(packet_tree* pkt_t){
  struct iphdr* oldest = get_minimum(pkt_t->rbt);
  packet_t* p = (packet_t*) (((char*) oldest) + (oldest->ihl << 2) + sizeof(struct udphdr));
  uint32_t id = ntohl(p->id);
  if(!oldest)
    return NULL;
  rb_tree_delete(pkt_t->rbt,&id); 
  pkt_t->npackts--;
  return oldest;
}

void dump_packet_tree(packet_tree* pkt_t){
  tree_iterator* it = new_tree_iterator(pkt_t->rbt);	
  struct iphdr* p;
  packet_t *pkt;

  printk("========================= TREE DUMP ===========================\n");
  while(tree_iterator_has_next(it)){			
    p = (struct iphdr*) tree_iterator_next(it);
    pkt = (packet_t*) (((char*) p) + (p->ihl << 2) + sizeof(struct udphdr));
    printk(">> Packet %u\n", ntohl(pkt->id));
  }							
  destroy_iterator(it);
  p = (struct iphdr*) get_minimum(pkt_t->rbt);
  if(p){
    pkt = (packet_t*) (((char*) p) + (p->ihl << 2) + sizeof(struct udphdr));
    printk("+) Tree minimum is %u\n", ntohl(pkt->id));
  }
  printk("===============================================================\n");
}
