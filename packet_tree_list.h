#ifndef __PACKET_TREE_LIST_H__
#define __PACKET_TREE_LIST_H__

#include "packet_tree.h"

typedef struct spkt_list_node{
  __be32 srcaddr;
  packet_tree* pkt_t;
  struct spkt_list_node* before;
  struct spkt_list_node* next;
}pkt_list_node;

typedef struct {
  pkt_list_node* first;
  pkt_list_node* last;
}pkt_list;

extern pkt_list* make_pkt_list(void);
extern packet_tree* append_packet_tree(pkt_list* l, __be32 srcaddr);
extern packet_tree* get_packet_tree(pkt_list* l, __be32 srcaddr);
extern void free_pkt_list(pkt_list* l);
#endif
