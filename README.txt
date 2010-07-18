Introduction
============
This module is used to timestamp packets arriving at a sink node.

There is no need for global synchronization. 
In fact, if absolute time is required then only the sink node's clock must be accurate. If relative time suffices then no clock is required to be accurate.

Requirements
============
* Netfilter enabled in the kernel

Packet structure
================
The module only works if the packets follow this structure:

* The first field must have 64 bits and will represent the accumulated time of the packet (see section below).
* The second fieald must have 32 bits and will represent the id of the packet.
* The following fields can be whatever comes into your mind.

Algorithm
=========
The algorithm is very simple. The first field in the packet's structure is used to record the accumulated processing time of each node the packet traverses.

This is achieve by timestamping the packet at arrival time. When the packet is ready to be sent to another node, another timestamp is taken and the arrival timestamp is subtracted to it.
Now we have the packet's time spent in the node. We now have to change the packet's first field to the value it already contained plus the packet's time spent in the node.

When this packet arrives at the sink node, its first field will contain an approximate value of the packet's living time. 
If at the exact moment the packet arrives at the sink we take a timestamp and subtract it the packet's accumulated time, we obtain the time at which the packet was created relative to the sink's clock.

Example:
Consider the following network where S is the sink node:

A------------>B----------->S

* Suppose A sends a packet P to S. 
* A starts by recording the time it took to process packet P. Lets say 4 units. This means that the first field of the packet will contain the value 4.
* It then sends P to B.
* B records the time when P arrived. Lets say 88. 
* When the packet is ready to be sent to S. B takes another timestamp. Lets say 90. This means that the processing time of P by B is 2 (90 - 88).
* B records this value in packet P's accumulated time. This means that the first field of P will contain 6 (4 + 2).
* When P arrives at S, another timestamp is taken. Lets say 156. Then S subtracts P's first field to this timestamp, yielding 156 - 6 = 150.
* This means that the packet P was created at 150 from S's point of view.

Implementation
==============
Most of the implementation code is commented so this section only provides a small explanation.

The idea behind the algorithm suggests that the packets must be timestamped at arrival. Instead of doing this in the packets structure, we've built an hash table in each node, with the following structure:

+--------------+
|              |
|              |
| first bucket------->+-----------------------+      +-----------------------+
| last bucket  |      | packet's arrival time |      | packet's arrival time |
|     |        |      | packet's source ip    |      | packet's source ip    |
|     |        |      | packet's source port  |      | packet's source port  |
|     |        |      | packet's id           |      | packet's id           |
|     |        |      | next bucket ---------------->| next bucket --------------> NULL
|     |        | NULL<-- previous bucket      |<------- previous bucket      |
|     |        |      +-----------------------+      +-----------------------+
|     |        |                                                            ^
|     +---------------------------------------------------------------------+
|              |
+--------------+

Note that the packet's id is required for hashing it into a bucket. This enables us to receive various packets from the same source.

Each row in the hash has a pointer to the first and last bucket. This was done to optimize insertion of new buckets.

Each bucket has a pointer to the next and previous bucket. This was done to optimize deletion of buckets.

A bucket stays in the hash as long as its corresponding packet stays in the node.

