# Nome do módulo
MODULENAME := cryptochannel

# Objetos que compõem o módulo (.c → .o)
obj-m := cryptochannel.o
cryptochannel-objs := src/cryptochannel.o src/crypto.o

# Diretórios
KDIR := /lib/modules/$(shell uname -r)/build
PWD  := $(shell pwd)
BUILD := $(PWD)/build

# Compilar o módulo
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Limpeza
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	rm -f /dev/$(MODULENAME)
	rm -rf $(BUILD)
