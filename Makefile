#_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/
#_/			Makefile for Monn File System Driver
#_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/_/

CFILES = moon.c sample.c
obj-m += moon.o

MODKO := moon.ko
KDIR  := /lib/modules/$(shell uname -r)/build

all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

install:
	insmod $(MODKO)

uninstall:
	rmmod $(MODKO)

reload:
	rmmod $(MODKO)
	insmod $(MODKO)

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

