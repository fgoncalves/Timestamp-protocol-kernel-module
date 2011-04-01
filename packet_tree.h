#ifndef __PACKET_TREE_H__
#define __PACKET_TREE_H__

#include <linux/ip.h>
#include "rbtree.h"

typedef struct {
  uint32_t npackts;
  tree_root* rbt;
}packet_tree;

extern packet_tree* new_packet_tree(void);
extern void free_packet_tree(packet_tree* pkt_t);
extern int8_t store_packet_in_tree(packet_tree* pkt_t, struct ip_hdr* pkt);
extern struct ip_hdr* get_packet_from_tree(packet_tree* pkt_t, uint32_t id);
extern void remove_packet_from_tree(packet_tree* pkt_t, uint32_t id);
extern struct ip_hdr* discard_oldest(packet_tree* pkt_t);
#endif
