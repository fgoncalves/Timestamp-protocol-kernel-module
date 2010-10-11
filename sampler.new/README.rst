Requirements
============
This program only works with more than 2 nodes if there is a routing protocol like B.A.T.M.A.N. working in the background.

Also, timestamps will only be correct if the timestamping module is inserted in the kernel. If not, the program will run without problems, but times will make no sense.

How to run it
=============
* If you want the node to act as a sink node just run it like this:

./sampler <ip-address> -s

* If you want the node to act as a collecting node just run it like this:

./sampler <ip-address>
  
|   Then you need to enter the number of packets to collect and the sink ip and port. For example:
|
|   ./sampler 127.0.0.1
|   1000
|   127.0.0.1:6666
|
|   This would send 1000 packets to the sink at 127.0.0.1:6666

Notice that there is no need to specify the port when the program is run. It will automatically bind to 57843.

Also, there is no need to enter "END" at the end of input.


New Features
===========
Now we can specify if we want to emulate the timestamp or not. Add -e flag if you want to emulate the timestamp.

Note that the sink node flag has changed from -sink to -s.

The output file will only be created at the sink node and it will always have the name ostatistics.data

Options
=======

[any number] - change the sampling rate to the given number. Default is 25 samples per second.

-e - Emulate timestamp.

-s - Make node act as a sink node

Example
=======
Suppose we want to send 100 packets to the sink node.

First we need to start the sink node:

./sampler 192.168.0.1 -s

Then we need to start the collector node:

|./sampler 192.168.0.1
|100
|192.168.0.1:57843

Makefile
========
The Makefile has changed a little bit. Now the program will only display errors if we compile it with -DDEBUG.

The Makefile has already this option, but if you don't need this behaviour you can changed it by editing CFLAGS and removing the -DDEBUG. If this is done, the generated code will not contain any assembly related to error messages, which reduces the program size.
