obj-m:=chrdevbase.o
# 指定编译好的内核的路径
KDIR?=/lib/modules/$(shell uname -r)/build
PWD:=$(shell pwd)

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -rf *.o *.ko .depend.* *.o