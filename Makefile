obj-m += timestamp.o
timestamp-objs := hooks.o utilities.o 

KERNEL_PATH=/lib/modules/$(shell uname -r)/build
#KERNEL_PATH = 2.6.24.4-gb9e83242-dirty

all:
	make -C $(KERNEL_PATH) M=$(PWD) modules
clean:
	make -C $(KERNEL_PATH) M=$(PWD) clean
