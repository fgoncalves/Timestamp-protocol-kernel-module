
#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef __MODULE__
#define __MODULE__
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/ip.h> 
#include <linux/icmp.h>
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
static uint64_t *buff;
static uint curr;
struct task_struct *rtt_task;

//This is the service port that the user level program must bind to
static unsigned short service_port = 57843;
module_param(service_port, ushort, 0000);
MODULE_PARM_DESC(service_port, "Destination port. Default 57843");

static int is_sink = 0;
module_param(is_sink, int, 0000);
MODULE_PARM_DESC(is_sink, "Set this to 1 if the node is a sink node. Default is 0.\n\t\t\tIf this is set to 0 in a sink node, a kthread will be created and the sink will try to measure rtt's sent to itself.");

static uint buff_size = 10;
module_param(buff_size, uint, 0000);
MODULE_PARM_DESC(buff_size, "Specifffy the size of the rtts buffer. Default is 10.");

int init_rtts_buffer(void){
  buff = alloc(buff_size, uint64_t);

  if(buff == NULL){
    print("Failed to alloc buffer.\n");
    return 1;
  }

  memset(buff, 0, buff_size * sizeof(uint64_t));

  curr = 0;
  return 0;
}

void store_rtt(uint64_t rtt){
  buff[curr] = rtt;

  if(curr == buff_size){
    curr = 0;
    return;
  }

  curr++;
  return;
}

/*These functions should take less time than rtt_it*/
uint64_t get_rtt_average(void){
  int i;
  uint64_t sum = 0;
  for(i = 0; i < buff_size; i++)
    sum += buff[i];
  //do_div stores the result in the first argument. It is needed because the kernel does not implement long long divisions.
  do_div(sum , buff_size);
  return sum;
}

uint64_t get_rtt_variance(void){
  uint64_t avg = get_rtt_average();
  uint64_t sum = 0;
  int i;
  for(i = 0; i < buff_size; i++)
    sum += ((buff[i] - avg) * (buff[i] - avg));
  //do_div stores the result in the first argument. It is needed because the kernel does not implement long long divisions.
  do_div(sum , buff_size);
  return sum;
}
/*=============================================================*/


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
  uint64_t avg_rtt, var_rtt;

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

    memcpy(&avg_rtt, transport_data + 16, 8);
    memcpy(&var_rtt, transport_data + 24, 8);
    avg_rtt = swap_time_byte_order(avg_rtt);
    var_rtt = swap_time_byte_order(var_rtt);

    print("%d: Pre routing received a packet with avg rtt %llu and variance %llu\n", __LINE__, avg_rtt, var_rtt);
    
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
  uint64_t avg_rtt, var_rtt;
  
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

  if((udp_header->dest) == (unsigned short) htons(service_port)){
    transport_data = skb->data + sizeof(struct iphdr) + sizeof(struct udphdr);
  
    memcpy(&acc_time, transport_data, 8);
    acc_time = swap_time_byte_order(acc_time);

    memcpy(&in_time, transport_data + 8, 8);
    in_time = swap_time_byte_order(in_time);

    //cpy received avg_rtt
    memcpy(&avg_rtt, transport_data + 16, 8);
    avg_rtt = swap_time_byte_order(avg_rtt);

    //from this point on acc_time will contain the total accumulated time
    kt = get_kernel_current_time();
    do_div(avg_rtt, 2);
    acc_time += (kt - in_time) + avg_rtt;
    print("%d: Post routing adde an average rtt of %llu ns\n", __LINE__, avg_rtt);
    if(acc_time < 0) {
      acc_time = 0;
    }

    avg_rtt = swap_time_byte_order(get_rtt_average());
    var_rtt = swap_time_byte_order(get_rtt_variance());

    memcpy(transport_data + 16, &avg_rtt, 8);
    memcpy(transport_data + 24, &var_rtt, 8);

    acc_time = swap_time_byte_order(acc_time);
    memcpy(skb->data + sizeof(struct iphdr) + sizeof(struct udphdr), &acc_time, 8);

    udp_header->check = 0;
    udp_header->check = udp_checksum(ip_header, udp_header, transport_data);
    if(!udp_header->check)
      udp_header->check = 0xFFFF;
  }
  
  return NF_ACCEPT;
}

void compute_rtt(s64 new_timestamp){
  //Just printing some values. I'm not doing the mean or standard deviation

  s64 rtt = get_kernel_current_time() - new_timestamp;

  store_rtt((uint64_t) rtt);
}

void dump_buffer(void){
  int i;
  for(i =0 ; i < buff_size; i++)
    print("[%d] %llu\n", i, buff[i]);
  print("Average: %llu ns\n", get_rtt_average());
  print("Variance: %llu ns\n", get_rtt_variance());
}

void handle_icmp(struct sk_buff *skb){
  struct iphdr* iph = ip_hdr(skb);
  struct icmphdr* icmph = (struct icmphdr*)(skb->data+(iph->ihl << 2));
  //|iph|icmph|iph|icmp|sent data ->icmp ttl exceeded
  //|iph|icmp|sent data ->icmp reply

  //check if it is an echo reply
  if(icmph->type == ICMP_ECHOREPLY){
    //ok maybe it's the reply from sink
    //lets check if we have 16 bytes of data

    if(ntohs(iph->tot_len) - sizeof(struct iphdr) - sizeof(struct icmphdr) == 16){
      //then grab the id and check its value
      s64 id = *((s64*) (skb->data + sizeof(struct iphdr) + sizeof(struct icmphdr)));
      if(swap_time_byte_order(id) != 0x00000000beefcafe)
	return;

      //grab the timestamp sent and compute rtt
      compute_rtt(swap_time_byte_order(*((s64*) (skb->data + sizeof(struct iphdr) + sizeof(struct icmphdr) + 8))));
    }
  }

  //check if ttl exceeded
  if(icmph->type == ICMP_TIME_EXCEEDED && icmph->code == ICMP_EXC_TTL){
    //lets get the encapsulated ip packet
    struct iphdr* enc_iph= (struct iphdr*) (skb->data + sizeof(struct iphdr) + sizeof(struct icmphdr));

    //is it an icmp echo request?
    if(enc_iph->protocol == IPPROTO_ICMP){
      struct icmphdr* enc_icmph = (struct icmphdr*) (skb->data + (sizeof(struct iphdr) << 1) + sizeof(struct icmphdr));

      if(enc_icmph->type == ICMP_ECHO && ntohs(enc_iph->tot_len) - sizeof(struct iphdr) - sizeof(struct icmphdr) >= 16){
	//then grab the id and check its value
	s64 id = *((s64*) (skb->data + (sizeof(struct iphdr) << 1) + (sizeof(struct icmphdr) << 1)));
	if(swap_time_byte_order(id) != 0x00000000beefcafe)
	  return;

	//grab the timestamp sent and compute rtt
	compute_rtt(swap_time_byte_order(*((s64*) (skb->data + (sizeof(struct iphdr) << 1) + (sizeof(struct icmphdr) << 1) + 8))));
      }
    }
  }
  return;
}

unsigned int nf_ip_local_in_hook(unsigned int hooknum, struct sk_buff *skb, const struct net_device *in, const struct net_device *out, int (*okfn)(struct sk_buff*)){
  struct iphdr* ip_header;
  struct udphdr* udp_header;
  unsigned char* transport_data;
  s64 in_time, acc_time = 0;
  uint64_t avg_rtt;

  if(!skb){ 
    return NF_ACCEPT; 
  } 
  if(!(skb->network_header) || !(skb->transport_header)){ 
    return NF_ACCEPT; 
  }
  
  ip_header = ip_hdr(skb);
  if(ip_header->protocol == IPPROTO_ICMP){
    handle_icmp(skb);
    return NF_ACCEPT;
  }

  if(ip_header->protocol != __udp_proto_id__){ 
    return NF_ACCEPT;
  }

  //  because there is a bug in the curr kernel we can't simple do "udp_header = udp_hdr(skb);". Instead we have to do:
  udp_header = (struct udphdr*)(skb->data+(ip_header->ihl << 2));

  // We're only interested in packets that have our service port.
  if((udp_header->dest) == (unsigned short) htons(service_port)){

    dump_buffer();

    transport_data = skb->data + sizeof(struct iphdr) + sizeof(struct udphdr);
    
    memcpy(&in_time, transport_data + 8, 8);
    in_time = swap_time_byte_order(in_time);

    memcpy(&acc_time, transport_data, 8);
    //from this point the second field is used to store the delay time
    memcpy(skb->data + sizeof(struct iphdr) + sizeof(struct udphdr) + 8, &acc_time, 8);
    acc_time = swap_time_byte_order(acc_time);

    memcpy(transport_data + 16, &avg_rtt, 8);
    avg_rtt = swap_time_byte_order(avg_rtt);

    //from this point on acc_time will contain the packet's creation time
    do_div(avg_rtt, 2);
    acc_time = in_time - acc_time - avg_rtt;    
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

  if(!is_sink){
    if(init_rtts_buffer()){
      cleanup_module();
      return 0;
    }
    rtt_task = kthread_run(send, NULL, "rtt-thread");
  }

  print("Packets are now being timestamped.\n");
  return 0;
}

void cleanup_module(){
  nf_unregister_hook(&nf_ip_pre_routing);
  nf_unregister_hook(&nf_ip_post_routing);
  nf_unregister_hook(&nf_ip_local_in);
  if(!is_sink)
    kthread_stop(rtt_task);
  print("Packets are no longer being timestamped.\n");
}


MODULE_LICENSE("GPL v2");
