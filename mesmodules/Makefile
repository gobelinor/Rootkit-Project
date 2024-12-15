obj-m += kernel_network.o
obj-m += hook.o
obj-m += kprobes_read.o
obj-m += kprobes_open.o
obj-m += cameleon.o
obj-m += hidemodule.o
obj-m += my_workqueue.o
obj-m += kill_all_communications.o
obj-m += kretprobes_getdents.o
obj-m += cameleon_2.o
obj-m += cameleon_ultimate.o
obj-m += hidefiles.o
obj-m += ipv4.o

all:
	make -C ../linux-6.10.10/ M=$(PWD) modules
	rm -vf *.o *.mod* modules.order Module.symvers .*
clean:
	make -C ../linux-6.10.10/ M=$(PWD) clean

