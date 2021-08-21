obj-m:=led.o

KDIR?=/lib/modules/$(shell uname -r)/build
PWD:=$(shell pwd)

modules:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	rm -rf *.o *.ko .depend.*