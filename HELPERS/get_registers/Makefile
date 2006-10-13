
CFLAGS+=-I$(HOME)/include/

ifneq ($(KERNELRELEASE),)
	obj-m := get_registers.o
else
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build
	PWD := $(shell pwd)

default:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
clean:
	-rm *.ko *.o .*.cmd Modules.symvers *.mod.c
	-rm -R .tmp_versions Module.symvers
endif

