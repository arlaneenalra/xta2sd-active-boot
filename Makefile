AS=nasm
ASFLAGS=-f bin

WATCOM_DOCKER=docker run --pull=missing --rm -t -v $(shell pwd):/src arlaneenalra/watcom-docker 

WATCOM_C=${WATCOM_DOCKER} wcc
WATCOM_CFLAGS=-0 -bt=dos

WATCOM_LD=${WATCOM_DOCKER} wlink
WATCOM_LDFLAGS=SYSTEM dos OPTION DOSSEG DEBUG DWARF \
							 ORDER CLNAME RDATA SEGMENT ResidentData \
							       CLNAME CODE SEGMENT ResidentCode SEGMENT ResidentEnd SEGMENT _TEXT SEGMENT BEGTEXT \
                     CLNAME FAR_DATA \
										 CLNAME BEGDATA SEGMENT _NULL SEGMENT _AFTERNULL \
										 CLNAME DATA \
										 CLNAME BSS \
										 CLNAME STACK

#WATCOM_LDFLAGS=SYSTEM dos OPTION DOSSEG DEBUG DWARF

.PHONY: all
all: boot/boot.bin tool/tool.exe

%.bin: %.s
	${AS} ${ASFLAGS} -o $@ -l $*.lst $^

%.exe: %.o
	${WATCOM_LD} NAME $@ ${WATCOM_LDFLAGS} OPTION MAP=$(patsubst %,%.map, $@) FILE { $^ }


%.o: %.c tool/tool.h
	${WATCOM_C} ${WATCOM_CFLAGS} -fo=$@ $(filter %.c, $^)

tool/boot.c: boot/boot.bin
	xxd -i $^ $@

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
	find . -iname '*.map' -exec rm {} \; 
	find . -iname '*.o' -exec rm {} \; 

	rm -f tool/boot.c

really_clean: clean
	rm -f real_boot.img
