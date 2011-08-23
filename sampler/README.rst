Requirements
============
This program only works with more than 2 nodes if there is a routing protocol like B.A.T.M.A.N. working in the background.

Also, timestamps will only be correct if the timestamping module is inserted in the kernel. If not, the program will run without problems, but times will make no sense.

How to run it
=============

Synopsis:

Usage: ./sampler -i <interface> [-r samples_per_second | -sp sink_port | -p port | -e] [-s] [-gps] [-in]

-i - Interface to bind the program
-r - Sample rate. Default is 25.
-sp - Sink port. Default is 57843. If you change this you also must change it in the timesamp.ko
-p - Port to bind. Default is 57843.
-e - Instead of getting time from OS, emulate it.
-s - This node will act as a sink.
-gps - Gather time from gps. This option does not disable the timestamping protocol.
-in - Indoor gps. Default is outdoor.

Example
=======
Suppose we want to send 100 packets to the sink node.

First we need to start the sink node:

./sampler -i eth0 -s

Then we need to start the collector node:

| ./sampler -i eth0
| 100
| 192.168.0.1:57843

The same example but with gps enabled.

./sampler -i eth0 -s

Then we need to start the collector node:

| ./sampler -i eth0 -gps
| 100
| 192.168.0.1:57843
