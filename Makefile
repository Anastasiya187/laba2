obj-m := kanal.o
KDIR := /home/ilya/linux-5.10.76
all:
	$(MAKE) -C $(KDIR) M=`pwd` modules
