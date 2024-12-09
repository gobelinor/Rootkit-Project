obj-m := hook.o
obj-m += cameleon_ultimate.o
obj-m += hello.o

all:
	make -C ../linux-6.10.10/ M=$(PWD) modules

clean:
	make -C ../linux-6.10.10/ M=$(PWD) clean
