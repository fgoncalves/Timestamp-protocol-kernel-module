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
#include "packet_tree_list.h"
#include "kpacket.h"

#define __udp_proto_id__ IPPROTO_UDP

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

static uint32_t max_packets = 20;
module_param(max_packets, uint, 0000);
MODULE_PARM_DESC(max_packets, "Maximum number of packets for each source that this node should buffer before discarding the oldest.");

pkt_list *packets_awaiting_air_time_estimation;
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

uint8_t have_previous_packet(packet_tree* pkt_t, struct iphdr* p){
  packet_t* pkt = APPLICATION_PAYLOAD(p);
  uint32_t id;
  if(!pkt_t){
    return 0;
  }

  dump_packet_tree(pkt_t);

  id = ntohl(pkt->id) - 1;
  if(get_packet_from_tree(pkt_t, htonl(id))){
    return 1;
  }

  return 0;
}

struct iphdr* get_previous_packet(packet_tree* pkt_t, struct iphdr* p){
  packet_t* pkt = APPLICATION_PAYLOAD(p);
  uint32_t id = ntohl(pkt->id) - 1;
  return get_packet_from_tree(pkt_t, htonl(id));
}

void store_packet_in_tree_list(packet_tree* pkt_t, struct iphdr* p){
  if(!pkt_t)
    pkt_t = append_packet_tree(packets_awaiting_air_time_estimation, p->saddr);

  if(!store_packet_in_tree(pkt_t, p))
    printk("Failed to store packet in tree\n");
}

uint8_t is_tree_full(packet_tree* pkt_t){
  if(!pkt_t)
    return 0;

  return (pkt_t->npackts == max_packets);
}

void dump_packet(uint8_t* bytes, uint16_t len){
  int i;
  for(i=0; i<len; i++)
    printk("%02X ", bytes[i]);
  printk("\n");
  printk("Packet has %u bytes\n", len);
}

uint8_t replace_packet_in_skb(struct sk_buff* skb, struct iphdr* p){
  int status;
  skb_reset_tail_pointer(skb);
  if(skb_tailroom(skb) < ntohs(p->tot_len)){
    if((status = pskb_expand_head(skb, 0, ntohs(p->tot_len), GFP_ATOMIC))){
      printk("pskb_expand_head failed. Error: %d\n", status);
      return 0;
    }
  }
  memcpy(skb_put(skb, ntohs(p->tot_len)), p,  ntohs(p->tot_len));
  return 1;
}

/*
 * This hook will run when a packet enters this node.
 */
unsigned int nf_ip_pre_routing_hook(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff*)){
  struct iphdr* ip_header, *prev_ip_header, *oldest;
  struct udphdr* udp_header;
  unsigned char* transport_data;
  s64 in_time = 0, air_time = 0;
  packet_t* pkt = NULL, *prev = NULL;
  packet_tree* pkt_t = NULL;

  if(!skb){ 
    return NF_ACCEPT; 
  } 
  if(!(skb->network_header) || !(skb->transport_header)){ 
    return NF_ACCEPT; 
  }
  
  // If packet is not UDP we don't need to check it.
  ip_header = ip_hdr(skb);
  if(ip_header->protocol != __udp_proto_id__){ 
    return NF_ACCEPT;
  }

  //  because there is a bug in the curr kernel we can't simply do "udp_header = udp_hdr(skb);". Instead we have to do:
  udp_header = (struct udphdr*)(skb->data+(ip_header->ihl << 2));

  // We're only interested in packets that have our service port as source port.
  if((udp_header->dest) == (unsigned short) htons(service_port)){ 
    in_time = get_kernel_current_time();

    in_time = swap_time_byte_order(in_time);

    transport_data = skb->data + sizeof(struct iphdr) + sizeof(struct udphdr);
    
    memcpy(transport_data + 8, & in_time, 8);

    udp_header->check = 0;
    udp_header->check = udp_checksum(ip_header, udp_header, transport_data);
    if(!udp_header->check)
      udp_header->check = 0xFFFF;
    
    pkt = APPLICATION_PAYLOAD(ip_header);
    pkt_t = get_packet_tree(packets_awaiting_air_time_estimation, ip_header->saddr);

    if(have_previous_packet(pkt_t, ip_header)){
      store_packet_in_tree(pkt_t, ip_header);

      prev_ip_header = get_previous_packet(pkt_t, ip_header);
      prev = APPLICATION_PAYLOAD(prev_ip_header);
      air_time = swap_time_byte_order(pkt->rtt);
      air_time *= 1000; //convert to ns
      in_time = swap_time_byte_order(prev->in_time);
      in_time += air_time;
      prev->in_time = swap_time_byte_order(in_time);      
      
      udp_header = (struct udphdr*) (((char*) prev_ip_header) + (prev_ip_header->ihl << 2));
      udp_header->check = 0;
      udp_header->check = udp_checksum(ip_header, udp_header, transport_data);
      if(!udp_header->check)
	udp_header->check = 0xFFFF;
 
      replace_packet_in_skb(skb, prev_ip_header);
      remove_packet_from_tree(pkt_t, prev->id);
      return NF_ACCEPT;
    }else{
      if(is_tree_full(pkt_t)){
	oldest = discard_oldest(pkt_t);
	if(oldest)
	  kfree(oldest);
      }

      store_packet_in_tree_list(pkt_t, ip_header);
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
  if(ip_header->protocol != __udp_proto_id__){ 
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
    kt = get_kernel_current_time();
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

  if(ip_header->protocol != __udp_proto_id__){ 
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

  if(!(packets_awaiting_air_time_estimation = make_pkt_list()))
    return 1;

  print("Packets are now being timestamped.\n");
  return 0;
}

void cleanup_module(){
  nf_unregister_hook(&nf_ip_pre_routing);
  nf_unregister_hook(&nf_ip_post_routing);
  nf_unregister_hook(&nf_ip_local_in);

  free_pkt_list(packets_awaiting_air_time_estimation);

  print("Packets are no longer being timestamped.\n");
}


MODULE_LICENSE("GPL v2");
