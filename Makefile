# Define o nome do modulo final (o .ko)
MODULENAME := cryptochannel

# Define quais objetos (arquivos .o) compõem o módulo.
# Por enquanto, apenas o arquivo principal.
cryptochannel-objs := src/cryptochannel.o

# O arquivo objeto final
obj-m := $(MODULENAME).o

# Localização do kernel sources (KDIR) e do diretorio atual (PWD)
KDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)

# Regra 'all': compila o módulo
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

# Regra 'clean': remove arquivos gerados
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean
	# Limpeza adicional (pode dar erro se o no nao existir, mas é seguro)
	sudo rm -f /dev/$(MODULENAME)
