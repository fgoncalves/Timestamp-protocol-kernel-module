#ifndef __RTT_THREAD__
#define __RTT_THREAD__

static char *sink_ip = "127.0.0.1";
module_param(sink_ip, charp, 0000);
MODULE_PARM_DESC(sink_ip, "Sink's ip");

static int rtt_it = 1000 /*ms*/;
module_param(rtt_it, int, 0000);
MODULE_PARM_DESC(rtt_it, "The rtt sending interval in milliseconds.");

static char *ifname = "eth0";
module_param(ifname, charp, 0000);
MODULE_PARM_DESC(ifname, "Interface name used to send rtt packets. Defaults to eth0");

extern int send(void* data);
#endif
