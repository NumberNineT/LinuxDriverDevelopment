obj-m:=newchrled.o
KDIR?=/lib/modules/$(shell uname -r)/build
PWD:=$(shell pwd)
CC=gcc

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
#echo $(MAKE)	# 就是make

clean:
	rm -rf *.o *.ko
