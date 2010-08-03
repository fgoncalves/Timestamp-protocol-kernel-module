obj-m = timestamping.o

KERNEL_PATH=$(shell uname -r)
#KERNEL_PATH = 2.6.24.4-gb9e83242-dirty

all:
	make -C /lib/modules/$(KERNEL_PATH)/build M=$(PWD) modules
clean:
	make -C /lib/modules/$(KERNEL_PATH)/build M=$(PWD) clean
