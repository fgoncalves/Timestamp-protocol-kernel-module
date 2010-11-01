
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
#include <linux/ip.h> 
#include <linux/netdevice.h> 
#include <linux/netfilter.h> 
#include <linux/netfilter_ipv4.h> 
#include <linux/skbuff.h> 
#include <linux/udp.h>
#include <linux/kthread.h>
#include "rtt_thread.h"
#include "utilities.h"

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

#ifdef NF_IP_LOCAL_IN
#define ip_local_in NF_IP_LOCAL_IN
#else
#define ip_local_in NF_INET_LOCAL_IN
#endif

static struct nf_hook_ops nf_ip_pre_routing;
static struct nf_hook_ops nf_ip_post_routing;
static struct nf_hook_ops nf_ip_local_in;

struct task_struct *rtt_task;

//This is the service port that the user level program must bind to
static unsigned short service_port = 57843;
module_param(service_port, ushort, 0000);
MODULE_PARM_DESC(service_port, "Destination port. Default 57843");

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

/*
 * This hook will run when a packet enters this node.
 */
unsigned int nf_ip_pre_routing_hook(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff*)){
  struct iphdr* ip_header;
  struct udphdr* udp_header;
  unsigned char* transport_data;
  s64 in_time = 0;

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

  //  because there is a bug in the current kernel we can't simply do "udp_header = udp_hdr(skb);". Instead we have to do:
  udp_header = (struct udphdr*)(skb->data+(ip_header->ihl << 2));

  // We're only interested in packets that have our service port as source port.
  if((udp_header->dest) == (unsigned short) htons(service_port)){ 
    in_time = get_kernel_current_time();
    print("%s:%d: pre routing hook took timestamp %lld\n", __FILE__, __LINE__, in_time);
    in_time = swap_time_byte_order(in_time);

    transport_data = skb->data + sizeof(struct iphdr) + sizeof(struct udphdr);
    
    memcpy(transport_data + 8, & in_time, 8);
    
    udp_header->check = 0;
    udp_header->check = udp_checksum(ip_header, udp_header, transport_data);
    if(!udp_header->check)
      udp_header->check = 0xFFFF;
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

  //  because there is a bug in the current kernel we can't simple do "udp_header = udp_hdr(skb);". Instead we have to do:
  udp_header = (struct udphdr*)(skb->data+(ip_header->ihl << 2));

  if((udp_header->dest) == (unsigned short) htons(service_port)){
    transport_data = skb->data + sizeof(struct iphdr) + sizeof(struct udphdr);
  
    memcpy(&acc_time, transport_data, 8);
    acc_time = swap_time_byte_order(acc_time);

    print("%s:%d: post routing hook read accumulated time %lld\n", __FILE__, __LINE__, acc_time);

    memcpy(&in_time, transport_data + 8, 8);
    in_time = swap_time_byte_order(in_time);

    print("%s:%d: post routing hook read timestamp %lld\n", __FILE__, __LINE__, in_time);

    //from this point on acc_time will contain the total accumulated time
    kt = get_kernel_current_time();
    print("%s:%d: post routing hook took timestamp %lld\n", __FILE__, __LINE__, kt);
    acc_time += (kt - in_time);
    if(acc_time < 0) {
      acc_time = 0;
    }

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
  
  // If packet is not UDP we don't need to check it.
  ip_header = ip_hdr(skb);
  if(ip_header->protocol != __udp_proto_id__){ 
    return NF_ACCEPT;
  }

  //  because there is a bug in the current kernel we can't simple do "udp_header = udp_hdr(skb);". Instead we have to do:
  udp_header = (struct udphdr*)(skb->data+(ip_header->ihl << 2));

  // We're only interested in packets that have our service port as source port.
  if((udp_header->dest) == (unsigned short) htons(service_port)){
    transport_data = skb->data + sizeof(struct iphdr) + sizeof(struct udphdr);
    
    memcpy(&in_time, transport_data + 8, 8);
    in_time = swap_time_byte_order(in_time);

    print("%s:%d: local routing hook read timestamp %lld\n", __FILE__, __LINE__, in_time);

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

  rtt_task = kthread_run(send, NULL, "rtt-thread");

  print("Packets are now being timestamped.\n");
  return 0;
}

void cleanup_module(){
  nf_unregister_hook(&nf_ip_pre_routing);
  nf_unregister_hook(&nf_ip_post_routing);
  nf_unregister_hook(&nf_ip_local_in);
  kthread_stop(rtt_task);
  print("Packets are no longer being timestamped.\n");
}


MODULE_LICENSE("GPL v2");
