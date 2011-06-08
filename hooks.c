#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef __MODULE__
#define __MODULE__
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ip.h> 
#include <linux/in.h>
#include <linux/icmp.h>
#include <linux/netdevice.h> 
#include <linux/netfilter.h> 
#include <linux/netfilter_ipv4.h> 
#include <linux/skbuff.h> 
#include <linux/udp.h>
#include <linux/kthread.h>
#include "utilities.h"
#include "kpacket.h"
#include "klist.h"

#define U2NS 1000

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

#ifdef NF_IP_LOCAL_IN
#define ip_local_in NF_IP_LOCAL_IN
#else
#define ip_local_in NF_INET_LOCAL_IN
#endif

#define APPLICATION_PAYLOAD(IP_PACKET)					\
  (packet_t*) (((char*) (IP_PACKET)) + ((IP_PACKET)->ihl << 2) + sizeof(struct udphdr))

static struct nf_hook_ops nf_ip_pre_routing;
static struct nf_hook_ops nf_ip_post_routing;
static struct nf_hook_ops nf_ip_local_in;

//This is the service port that the user level program must bind to
static unsigned short service_port = 57843;
module_param(service_port, ushort, 0000);
MODULE_PARM_DESC(service_port, "Destination port. Default 57843");

klist *packets_awaiting_air_time_estimation;
/*
 * Because we couldn't find a udp checksum function in the kernel libs we've built one ourselves.
 *
 * This may be changed in the future.
 */
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

  dealloc(padded_data);
  
  return (__be16) ~sum;
}

//###  Change skb by removing its ip packet and replacing by the given one. The idea is to replace it be the previous one with air time estimated and tell netfilters to accept the packet. ###
//---
//   + skb - Socket buffer to replace ip packet.
//   + p - New ip packet.
//> **Return**: 1 if succeeded, 0 otherwise.
uint8_t replace_packet_in_skb(struct sk_buff* skb, struct iphdr* p){
  int status;
  debug("Replacing skb ip packet.\n");
  skb_reset_tail_pointer(skb);
  if(skb_tailroom(skb) < ntohs(p->tot_len)){
    debug("I need to expand skb tail room to %uB.\n", ntohs(p->tot_len));
    if((status = pskb_expand_head(skb, 0, ntohs(p->tot_len) - skb_tailroom(skb), GFP_ATOMIC))){
      printk("pskb_expand_head failed. Error: %d\n", status);
      return 0;
    }
    debug("I was able to expand skb tail room and now we have %uB.\n", skb_tailroom(skb));
  }
  memcpy(skb_put(skb, ntohs(p->tot_len)), p,  ntohs(p->tot_len));
  skb_set_network_header(skb, 0);
  debug("Skb should contain a new packet.\n");
  return 1;
}

//###  Add a packet to the air time estimation list based on its source ip. If a previous packet exists it will be returned. ###
//---
//   + kl - List that keeps packets awaiting air time estimation.
//   + new - Ip packet received now.
//> **Return**: If exists, the previous packet will be return. This packet has not yet been delivered to the application layer. Otherwise, NULL is returned.
static struct iphdr* add_packet_to_list(klist* kl, struct iphdr* new){
  klist_iterator* it = make_klist_iterator(kl);
  struct iphdr *n;
  klist_node* i;
  char* data;
  printk("Adding packet to list.\n");
  while(klist_iterator_has_next(it)){
    i = klist_iterator_next(it);
    n = (struct iphdr*) i->data;
    if(n->saddr == new->saddr){
      printk("I've found a previous packet!!\n");
      data = kmalloc(ntohs(new->tot_len), GFP_ATOMIC);
      if(!data){
	printk("%s in %s:%u: kmalloc failed. Unable to add packet to list.\n", __FUNCTION__, __FILE__, __LINE__);
	return NULL;
      }
      memcpy(data, new, ntohs(new->tot_len));
      i->data = data;
      free_klist_iterator(it);
      return n;
    }
  }
  free_klist_iterator(it);
  printk("I could not find a pevious packet. I'm going to add this one");
  data = kmalloc(ntohs(new->tot_len), GFP_ATOMIC);
  if(!data){
    printk("%s in %s:%u: kmalloc failed. Unable to add packet to list.\n", __FUNCTION__, __FILE__, __LINE__);
    return NULL;
  }
  memcpy(data, new, ntohs(new->tot_len));

  add_klist_node_to_klist(kl, make_klist_node(data));
  printk("OK, I've added the packet.\n");
  return NULL;
}

/*
 * This hook will run when a packet enters this node.
 */
unsigned int nf_ip_pre_routing_hook(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff*)){
  struct iphdr* ip_header, *prev_ip_header;
  struct udphdr* udp_header;
  unsigned char* transport_data;
  s64 in_time = 0, air_time = 0;
  packet_t* pkt = NULL, *prev = NULL;
  
  if(!skb){ 
    return NF_ACCEPT; 
  } 
  if(!(skb->network_header) || !(skb->transport_header)){ 
    return NF_ACCEPT; 
  }
  
  // If packet is not UDP we don't need to check it.
  ip_header = ip_hdr(skb);
  if(ip_header->protocol != IPPROTO_UDP){ 
    return NF_ACCEPT;
  }

  udp_header = (struct udphdr*)(((char*) ip_header) + (ip_header->ihl << 2));

  // We're only interested in packets that have our service port as source port.
  if((udp_header->dest) == (unsigned short) htons(service_port)){ 
    in_time = get_us_kernel_current_time_in_ns();

    in_time = swap_time_byte_order(in_time);

    transport_data = skb->data + sizeof(struct iphdr) + sizeof(struct udphdr);
    
    memcpy(transport_data + 8, & in_time, 8);

    udp_header->check = 0;
    udp_header->check = udp_checksum(ip_header, udp_header, transport_data);
    if(!udp_header->check)
      udp_header->check = 0xFFFF;
    
    pkt = APPLICATION_PAYLOAD(ip_header);
    prev_ip_header = add_packet_to_list(packets_awaiting_air_time_estimation, ip_header);

    if(prev_ip_header){
      printk("This is great, add_packet_to_list found a previous packet.\n");
      prev = APPLICATION_PAYLOAD(prev_ip_header);
      air_time = swap_time_byte_order(pkt->rtt);
      air_time *= U2NS; //convert to ns
      printk("Air time (This better be zero): %lld\n", air_time);
      in_time = swap_time_byte_order(prev->in_time);
      in_time += air_time;
      prev->in_time = swap_time_byte_order(in_time);      
      
      udp_header = (struct udphdr*) (((char*) prev_ip_header) + (prev_ip_header->ihl << 2));
      udp_header->check = udp_checksum(prev_ip_header, udp_header, transport_data);
      if(!udp_header->check)
	udp_header->check = 0xFFFF;
 
      replace_packet_in_skb(skb, prev_ip_header);
      return NF_ACCEPT;
    }else{
      printk("add_packet_to_list did not find a previous packet. I'm stealing this socket buffer.\n");
      kfree_skb(skb);
      return NF_STOLEN;
    }
  }
  
  return NF_ACCEPT;
}

/*
 * This hook will run when a packet is ready to exit this node.
 */
unsigned int nf_ip_post_routing_hook(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff*)){
  struct iphdr* ip_header;
  struct udphdr* udp_header;
  unsigned char* transport_data;
  s64 acc_time, in_time, kt;
  
  if(!skb){ 
    return NF_ACCEPT; 
  } 
  if(!(skb->network_header) || !(skb->transport_header)){ 
    return NF_ACCEPT; 
  }
  
  ip_header = ip_hdr(skb);
  if(ip_header->protocol != IPPROTO_UDP){ 
    return NF_ACCEPT;
  }

  udp_header = (struct udphdr*)(skb->data+(ip_header->ihl << 2));

  if((udp_header->dest) == (unsigned short) htons(service_port)){
    transport_data = skb->data + sizeof(struct iphdr) + sizeof(struct udphdr);
  
    memcpy(&acc_time, transport_data, 8);
    acc_time = swap_time_byte_order(acc_time);

    memcpy(&in_time, transport_data + 8, 8);
    in_time = swap_time_byte_order(in_time);

    //from this point on acc_time will contain the total accumulated time
    kt = get_us_kernel_current_time_in_ns();
    acc_time += (kt - in_time);

    /*I really need to check this crap!!!!*/
    if(acc_time < 0) {
      acc_time = 0;
    }
    /*=====================================*/

    acc_time = swap_time_byte_order(acc_time);
    memcpy(skb->data + sizeof(struct iphdr) + sizeof(struct udphdr), &acc_time, 8);

    udp_header->check = 0;
    udp_header->check = udp_checksum(ip_header, udp_header, transport_data);
    if(!udp_header->check)
      udp_header->check = 0xFFFF;
  }
  
  return NF_ACCEPT;
}

unsigned int nf_ip_local_in_hook(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff*)){
  struct iphdr* ip_header;
  struct udphdr* udp_header;
  unsigned char* transport_data;
  s64 in_time, acc_time = 0;

  if(!skb){ 
    return NF_ACCEPT; 
  } 
  if(!(skb->network_header) || !(skb->transport_header)){ 
    return NF_ACCEPT; 
  }
  
  ip_header = ip_hdr(skb);

  if(ip_header->protocol != IPPROTO_UDP){ 
    return NF_ACCEPT;
  }

  //  because there is a bug in the curr kernel we can't simple do "udp_header = udp_hdr(skb);". Instead we have to do:
  udp_header = (struct udphdr*)(skb->data+(ip_header->ihl << 2));

  // We're only interested in packets that have our service port.
  if((udp_header->dest) == (unsigned short) htons(service_port)){
    transport_data = skb->data + sizeof(struct iphdr) + sizeof(struct udphdr);
    
    memcpy(&in_time, transport_data + 8, 8);
    in_time = swap_time_byte_order(in_time);

    memcpy(&acc_time, transport_data, 8);
    //from this point the second field is used to store the delay time
    memcpy(skb->data + sizeof(struct iphdr) + sizeof(struct udphdr) + 8, &acc_time, 8);
    acc_time = swap_time_byte_order(acc_time);

    //from this point on acc_time will contain the packet's creation time
    acc_time = in_time - acc_time;
    acc_time = swap_time_byte_order(acc_time);

    memcpy(skb->data + sizeof(struct iphdr) + sizeof(struct udphdr), &acc_time, 8);

    udp_header->check = 0;
    udp_header->check = udp_checksum(ip_header, udp_header, transport_data);
    if(!udp_header->check)
      udp_header->check = 0xFFFF;
  }
  
  return NF_ACCEPT;
}

int init_module(){
  if(!(packets_awaiting_air_time_estimation = make_klist()))
    return 1;

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

  nf_ip_local_in.hook = nf_ip_local_in_hook;
  nf_ip_local_in.pf = PF_INET;
  nf_ip_local_in.hooknum = ip_local_in;
  nf_ip_local_in.priority = NF_IP_PRI_FIRST;
  nf_register_hook(& nf_ip_local_in);

  print("Packets are now being timestamped.\n");
  return 0;
}

void free_l_node(void* data){
  kfree(data);
}

void cleanup_module(){
  nf_unregister_hook(&nf_ip_pre_routing);
  nf_unregister_hook(&nf_ip_post_routing);
  nf_unregister_hook(&nf_ip_local_in);

  free_klist(packets_awaiting_air_time_estimation, free_l_node);

  print("Packets are no longer being timestamped.\n");
}


MODULE_LICENSE("GPL v2");
