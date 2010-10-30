#ifndef __KERNEL__
#define __KERNEL__
#endif

#ifndef __MODULE__
#define __MODULE__
#endif

#include <linux/kernel.h>
#include <linux/moduleparam.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/icmp.h>
#include <linux/ip.h>
#include <linux/skbuff.h>
#include <linux/netdevice.h>
#include <net/net_namespace.h>
#include "rtt_thread.h"
#include "utilities.h"

struct net_device* get_interface(void){
  struct net_device* dev = dev_get_by_name(&init_net, ifname);
  
  if(!dev)
    print("%s:%d: Unnable to obtain interface with name %s\n", __FILE__, __LINE__, ifname);
  else
    print("Sending rtt packets through %s\n", dev->name); 

  return dev;
}

int send(void* data){
  struct net_device *dev = get_interface();


  while(1){
    


    msleep(rtt_it);

    if (kthread_should_stop())
      break;
  }

  dev_put(dev);
  return 0;
}
