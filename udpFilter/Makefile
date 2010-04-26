obj-m += udpRecvCallback.o
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

all: kernel user

kernel:
	$(MAKE) -C $(KDIR) SUBDIRS=$(PWD) modules

user:
	$(CC) -o udpUser udpUser.c

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	rm -f *~
