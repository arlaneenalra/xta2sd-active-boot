AS=nasm
ASFLAGS=-f bin

WATCOM=docker run --rm -t -v $(shell pwd):/src dockerhy/watcom-docker:latest 

WATCOM_C=${WATCOM} wcl
WATCOM_CFLAGS=-0 -bc -bt=dos 

.PHONY: all
all: boot/boot.bin tool/tool.exe

%.bin: %.s
	${AS} ${ASFLAGS} -o $@ -l $*.lst $^

%.exe: %.c tool/tool.h
	${WATCOM_C} ${WATCOM_CFLAGS} -fe=$(patsubst %,/src/%, $@) $(patsubst %, /src/%, $(filter %.c, $^))


tool/boot.c: boot/boot.bin
	xxd -i $^ $@

tool/tool.exe: tool/tool.c tool/boot.c tool/disk.c

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
