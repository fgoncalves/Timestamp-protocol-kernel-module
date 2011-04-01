#include "packet_tree_list.h"

static pkt_list_node* make_pkt_node(__be32 srcaddr){
  pkt_list_node* pn = (pkt_list_node*) kmalloc(sizeof(pkt_list_node));

  if(!pn){
    printk("%s in %s:%d: kmalloc failed\n", __FUNCTION__, __FILE__, __LINE__);
    return NULL;
  }

  memset(pn, 0, sizeof(pkt_list_node));
  pn->srcaddr = srcaddr;
  pn->pkt_t = new_packet_tree();
  return pn;
}

pkt_list* make_pkt_list(void){
  pkt_list* pn = (pkt_list*) kmalloc(sizeof(pkt_list));

  if(!pn){
    printk("%s in %s:%d: kmalloc failed\n", __FUNCTION__, __FILE__, __LINE__);
    return NULL;
  }

  memset(pn, 0, sizeof(pkt_list));
  return pn;
}

packet_tree* append_packet_tree(pkt_list* l, __be32 srcaddr){
  if(!l->first){
    l->first = make_pkt_node(srcaddr);
    if(!l->first)
      return NULL;
    l->last = l->first;
    return l->first->pkt_t;
  }

  l->last->next = make_pkt_node(srcaddr);
  if(!l->last->next)
    return NULL;
  l->last->next->before = l->last;
  l->last = last->next;

  return l->last->pkt_t;
}

packet_tree* get_packet_tree(pkt_list* l, __be32 srcaddr){
  pkt_list_node* npkt = NULL;
  for(npkt = l->first; pkt != NULL; pkt = pkt->next)
    if(pkt->srcaddr == srcaddr)
      return pkt->pkt_t;
  return NULL;
}

void free_pkt_list(pkt_list* l){
  pkt_list_node* npkt = NULL;
  npkt = l->first; 
  while(pkt != NULL){
    l->first = npkt->next;
    free_packet_tree(pkt->pkt_t);
    kfree(npkt);
    npkt = l->first; 
  }
  kfree(l);
}
