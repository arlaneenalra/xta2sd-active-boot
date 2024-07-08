AS=nasm
ASFLAGS=-f bin

#WATCOM=docker run --rm -t -v $(shell pwd):/src dockerhy/watcom-docker:latest 
WATCOM=docker run --rm -t -v $(shell pwd):/src watcom

WATCOM_C=${WATCOM} wcl
#WATCOM_CFLAGS=-0 -bc -bt=dos 
WATCOM_CFLAGS=-0 -bt=dos -c

WATCOM_LD=${WATCOM} wlink
#WATCOM_LDFLAGS=SYSTEM dos ORDER CLNAME RDATA SEGMENT ResidentData CLNAME CODE
WATCOM_LDFLAGS=SYSTEM dos 

.PHONY: all
all: boot/boot.bin tool/tool.exe

%.bin: %.s
	${AS} ${ASFLAGS} -o $@ -l $*.lst $^

%.exe: %.o
	${WATCOM_LD} ${WATCOM_LDFLAGS} NAME $(patsubst %,/src/%, $@) FILE {$(patsubst %, /src/%, $^)}
#	${WATCOM_C} ${WATCOM_CFLAGS} -fe=$(patsubst %,/src/%, $@) $(patsubst %, /src/%, $(filter %.c, $^))


%.o: %.c tool/tool.h
	${WATCOM_C} ${WATCOM_CFLAGS} -fo=$(patsubst %,/src/%, $@) $(patsubst %, /src/%, $(filter %.c, $^))

tool/boot.c: boot/boot.bin
	xxd -i $^ $@

#tool/tool.exe: tool/resident.c tool/tool.c tool/boot.c tool/disk.c tool/tsr.c
tool/tool.exe: tool/resident.o tool/tool.o tool/boot.o tool/disk.o tool/tsr.o


.PHONY: real_image.img
real_image.img: boot.img boot/boot.bin
	cat ./boot/boot.bin ./boot.img > ./real_boot.img

.PHONY: clean
clean:
	find . -iname '*.bin' -exec rm {} \;
	find . -iname '*.lst' -exec rm {} \; 
	find . -iname '*.com' -exec rm {} \; 
	find . -iname '*.exe' -exec rm {} \; 
	find . -iname '*.o' -exec rm {} \; 

	rm -f tool/boot.c

really_clean: clean
	rm -f real_boot.img
