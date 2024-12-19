# Nom du module
obj-m += ipv5.o

# Détection automatique du répertoire de build du noyau
KERNEL_VERSION := $(shell uname -r)
KERNELDIR ?= /lib/modules/$(KERNEL_VERSION)/build

# Optionnel : chemin personnalisé pour les sources du noyau
LINUX_SRC ?= ../../linux-6.10.10/

# Cible par défaut
all:
ifeq ($(wildcard $(LINUX_SRC)/Makefile),)
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules
else
	$(MAKE) -C $(LINUX_SRC) M=$(PWD) modules
endif
	rm -vf *.o *.mod.c *.mod modules.order Module.symvers
