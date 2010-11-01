#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef __MODULE__
#define __MODULE__
#endif

#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kthread.h>
#include <linux/icmp.h>
#include <linux/ip.h>
#include <linux/skbuff.h>
#include <linux/net.h>
#include <linux/in.h>
#include <linux/socket.h>
#include <linux/inet.h>
#include "rtt_thread.h"
#include "utilities.h"

static int rtt_it = 1000 /*ms*/;
static char *sink_ip = "127.0.0.1";
static char *src_ip = "127.0.0.1";

module_param(sink_ip, charp, 0000);
MODULE_PARM_DESC(sink_ip, "Sink's ip");
module_param(src_ip, charp, 0000);
MODULE_PARM_DESC(src_ip, "Source ip");
module_param(rtt_it, int, 0000);
MODULE_PARM_DESC(rtt_it, "The rtt sending interval in milliseconds.");

struct socket * set_up_icmp_socket(void) {
  struct socket * icmp_sock;
  struct sockaddr_in sin;
  int error;

  error = sock_create(PF_INET, SOCK_RAW, IPPROTO_ICMP,&icmp_sock);
  if (error < 0) { 
    print("%s:%d: Unnable to create rtt socket\n", __FILE__, __LINE__);
    return null;
  } 

  sin.sin_family = AF_INET;
  sin.sin_addr.s_addr = in_aton(sink_ip);
  sin.sin_port = htons(6666); //This doesn't really matter

  error = icmp_sock->ops->connect(icmp_sock,(struct sockaddr*)&sin,sizeof(sin),0);
  if (error < 0) {
    print("%s:%d: Error connecting client socket to server\n", __FILE__, __LINE__);
    return null;
  }

  return icmp_sock;
}

int send_bytes(struct socket *sock, char *buffer, int length){
  struct msghdr msg;
  mm_segment_t oldfs;
  struct iovec iov; 
  int len2;

  msg.msg_name = null;
  msg.msg_namelen = 0;
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;
  msg.msg_control = null;
  msg.msg_controllen = 0;

  msg.msg_flags = MSG_NOSIGNAL; //don't wait

  iov.iov_base = buffer; 
  iov.iov_len = (__kernel_size_t) length;

  oldfs = get_fs(); 
  set_fs(KERNEL_DS);

  len2 = sock_sendmsg(sock, &msg, (size_t)(length));

  set_fs(oldfs);

  return len2;
}

unsigned short csum (unsigned short *buf, int nwords){
  unsigned long sum;
  for (sum = 0; nwords > 0; nwords--)
    sum += *buf++;
  sum = (sum >> 16) + (sum & 0xffff);
  sum += (sum >> 16);
  return ~sum;
}

void dump_packet(char* pack, int offset, int size){
  for(; offset < size; offset++){
    print("%02X ", pack[offset]);
  }
  print("\n");
}

int send(void* data){
  struct socket* sock = set_up_icmp_socket();
  int length = sizeof(struct iphdr) + sizeof(struct icmphdr);
  char packet[length];
  struct iphdr* ip = (struct iphdr*) packet;
  struct icmphdr* icmp = (struct icmphdr*) (packet + sizeof(struct iphdr));
  char __user one = 1;
  char __user *val = &one; //ok... I copied this from a site. Hope it works

  print("Sink node ip: %s\n", sink_ip);
  print("Source node ip: %s\n", src_ip);

  if(!sock)
    return -1;

  ip->ihl = 5; 
  ip->version = 4;
  ip->tos = 0;
  ip->tot_len = length;
  ip->id = 0;
  ip->frag_off = 0;
  ip->ttl = 0;
  ip->protocol = IPPROTO_ICMP;
  ip->check = 0;
  ip->saddr = in_aton(src_ip);
  ip->daddr = in_aton(sink_ip);

  icmp->type = ICMP_ECHO;
  icmp->code = 0;
  icmp->checksum = 0;
  icmp->un.echo.id = 0;
  icmp->checksum = csum((unsigned short*)(packet + sizeof(struct iphdr)), 4);
  
  ip->check = csum((unsigned short *) packet, ip->tot_len >> 1);

  if(sock->ops->setsockopt(sock, IPPROTO_IP, IP_HDRINCL, val, sizeof (int)) < 0){
    print("%s:%d: Cannot set IP_HDRINCL. Stoping rtt thread.\n", __FILE__, __LINE__);
    return -1;
  }

  while(1){
    send_bytes(sock, packet, length);
    msleep(rtt_it);
    if (kthread_should_stop())
      break;
  }

  return 0;
}
