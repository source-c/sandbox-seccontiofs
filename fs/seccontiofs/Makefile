SECCONTIOFS_VERSION="0.1"

EXTRA_CFLAGS += -DSECCONTIOFS_VERSION=\"$(SECCONTIOFS_VERSION)\"

obj-$(CONFIG_SECCONTIO_FS) += seccontiofs.o

seccontiofs-y := dentry.o file.o inode.o main.o super.o lookup.o mmap.o

export CONFIG_SECCONTIO_FS=m

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
	$(RM) *~

.PHONY: all clean