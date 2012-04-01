obj-m = upb.o

upb-objs = \
  setjmp.o \
  ../../upb/upb.o \
  ../../upb/bytestream.o \
  ../../upb/def.o \
  ../../upb/handlers.o \
  ../../upb/table.o \
  ../../upb/refcount.o \
  ../../upb/msg.o \
  ../../upb/pb/decoder.o \
  ../../upb/pb/textprinter.o \
  ../../upb/pb/varint.o \

KVERSION = $(shell uname -r)

ccflags-y := -I$(PWD) -I$(PWD)/../.. -Wno-declaration-after-statement -std=gnu99

all:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(KVERSION)/build M=$(PWD) clean
