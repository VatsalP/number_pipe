# To build modules outside of the kernel tree, we run "make"
# in the kernel source tree; the Makefile there then includes this
# Makefile once again.

# This conditional selects whether we are being included from the
# kernel Makefile or not.
ifeq ($(KERNELRELEASE),)

	# Assume the source tree is where the running kernel was built
	# You should set KERNELDIR in the environment if it's elsewhere
	KERNELDIR ?= /lib/modules/$(shell uname -r)/build

	# The current directory is passed to sub-makes as argument
	PWD := $(shell pwd)

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules

modules_install:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules_install

producer: producer_numbers.c
	gcc -o producer_numbers producer_numbers.c

consumer: consumer_numbers.c
	gcc -o consumer_numbers consumer_numbers.c

clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions producer_numbers consumer_numbers *.symvers *.order

.PHONY: modules modules_install clean

else
	# called from kernel build system: just declare what our modules are
	obj-m := number_pipe.o
endif